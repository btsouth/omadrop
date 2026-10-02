use std::io::{IsTerminal, Read};
use std::process::ExitCode;

use clap::Parser;

use ttfx::engine::terminal::Terminal;
use ttfx::{cli, engine};

fn get_piped_input() -> String {
    let stdin = std::io::stdin();
    if stdin.is_terminal() {
        return String::new();
    }
    let mut buf: Vec<u8> = Vec::new();
    if stdin.lock().read_to_end(&mut buf).is_err() {
        return String::new();
    }
    // strict UTF-8, like Python's text-mode stdin (plan.md §8)
    match String::from_utf8(buf) {
        Ok(s) => s,
        Err(e) => {
            ttfx::outln!("Error decoding input: {e}");
            std::process::exit(1);
        }
    }
}

/// Skip the arena teardown on the way out. Output is already flushed and
/// nothing in the engine has a Drop impl that does work, so freeing tens of
/// thousands of characters — each with its own scenes, paths and frames — one
/// by one is pure exit latency; on binarypath it is ~4% of the run.
///
/// Only the exit paths come through here. A resize rebuild drops its engine
/// normally, so a long session of resizes does not accumulate them.
fn forget_engine<E, C>(effect: E, ctx: C) {
    std::mem::forget(effect);
    std::mem::forget(ctx);
}

fn main() -> ExitCode {
    ttfx::tune_allocator();
    ttfx::restore_sigpipe();
    let cli = cli::Cli::parse();

    // upstream prints the completion script and returns before any input handling
    if let Some(shell) = &cli.print_completion {
        use clap::CommandFactory;
        let generator = match shell.as_str() {
            "bash" => clap_complete::Shell::Bash,
            _ => clap_complete::Shell::Zsh,
        };
        let mut command = cli::Cli::command();
        clap_complete::generate(generator, &mut command, "ttfx", &mut std::io::stdout());
        return ExitCode::SUCCESS;
    }

    let input_data = match &cli.input_file {
        Some(path) => match std::fs::read(path) {
            Ok(bytes) => match String::from_utf8(bytes) {
                Ok(s) => s,
                Err(e) => {
                    // upstream prints runtime file errors to STDOUT and exits 1
                    ttfx::outln!("Error reading input file: {e}");
                    return ExitCode::from(1);
                }
            },
            Err(e) => {
                ttfx::outln!("Error reading input file: {e}");
                return ExitCode::from(1);
            }
        },
        None => get_piped_input(),
    };

    if input_data.trim().is_empty() {
        ttfx::outln!("NO INPUT.");
        return ExitCode::from(1);
    }

    if cli.m0_dump {
        return m0_dump(&input_data, &cli);
    }

    if cli.music.is_some() {
        return run_music(&cli, &input_data);
    }

    let mut rng = match cli.seed {
        Some(seed) => ttfx::utils::rng::Rng::seeded(seed),
        None => ttfx::utils::rng::Rng::from_entropy(),
    };

    // --random-effect: pick from the registry (filtered), run with pure
    // default effect config — upstream ignores effect CLI args here too.
    let chosen_effect;
    let effect_command = if cli.random_effect {
        use clap::CommandFactory;
        let mut names: Vec<String> = cli::Cli::command()
            .get_subcommands()
            .map(|c| c.get_name().to_string())
            .collect();
        if !cli.include_effects.is_empty() {
            names.retain(|n| cli.include_effects.contains(n));
        }
        names.retain(|n| !cli.exclude_effects.contains(n));
        if names.is_empty() {
            ttfx::errln!("Error: No effects available after filtering.");
            return ExitCode::from(1);
        }
        let name = names[rng.choice_index(names.len())].clone();
        chosen_effect = match clap::Parser::try_parse_from::<_, &str>(["ttfx", &name]) {
            Ok(cli::Cli {
                effect: Some(effect),
                ..
            }) => effect,
            _ => {
                ttfx::errln!("Error: failed to build effect '{name}'.");
                return ExitCode::from(1);
            }
        };
        &chosen_effect
    } else {
        match &cli.effect {
            Some(effect) => effect,
            None => {
                ttfx::errln!("Error: No effect specified.");
                return ExitCode::from(1);
            }
        }
    };

    let mut config = cli.terminal_config();
    // SIGWINCH is delivered to every process in the terminal's foreground group,
    // whatever its stdout points at. Reacting to it when the animation is being
    // redirected would leave a truncated first run followed by a complete second
    // one in the file. SIGTERM teardown is tty-only for the same reason: a
    // redirected stream must not gain teardown bytes. Only the teardown differs
    // — the tty run re-raises afterwards, so both die from the signal.
    let tty_output = !cli.parity_dump && std::io::stdout().is_terminal();
    if !cli.parity_dump {
        ttfx::install_sigint_handler();
    }
    if tty_output {
        ttfx::install_sigterm_handler();
        ttfx::install_sigwinch_handler();
    }

    // The engines in the order they are offered the run: fx, then the
    // original engine, which takes what fx declines (out-of-range duration
    // options) and every run under TTFX_FX=0. The original engine is also the
    // reference fx is checked against.
    let result = loop {
        let clock = if cli.parity_dump || cli.virtual_clock {
            ttfx::engine::ctx::Clock::virtual_with_frame_rate(config.frame_rate)
        } else {
            ttfx::engine::ctx::Clock::real()
        };
        if let Some(mut effect) = ttfx::fx::effects::build(effect_command) {
            let mut engine = match ttfx::fx::Engine::new(&input_data, config.clone(), rng, clock) {
                Ok(engine) => engine,
                Err(engine::error::EngineError::UnsupportedAnsiSequence(seq)) => {
                    ttfx::errln!("Error: Unsupported ANSI sequence in input data: {seq:?}");
                    return ExitCode::from(1);
                }
                Err(e) => {
                    ttfx::errln!("Error: {e}");
                    return ExitCode::from(1);
                }
            };
            let outcome = if cli.parity_dump {
                ttfx::fx::run::dump_effect(effect.as_mut(), &mut engine, cli.max_frames)
                    .map(|_| ttfx::engine::effect::RunOutcome::Complete)
            } else {
                ttfx::fx::run::run_effect(effect.as_mut(), &mut engine, tty_output)
            };
            match outcome {
                Ok(ttfx::engine::effect::RunOutcome::TerminalResized) => {
                    config.reuse_canvas = false;
                    rng = engine.rng;
                    continue;
                }
                done => {
                    forget_engine(effect, engine);
                    break done.map(|_| ());
                }
            }
        }
        // TTFX_FX=force: a run fx declines is an error, so comparisons can't
        // quietly check the original engine against itself.
        if std::env::var_os("TTFX_FX").is_some_and(|v| v == "force") {
            ttfx::errln!("ttfx: TTFX_FX=force, but the fx engine declined this run");
            return ExitCode::from(3);
        }
        let mut ctx =
            match ttfx::engine::ctx::EngineCtx::new(&input_data, config.clone(), rng, clock) {
                Ok(ctx) => ctx,
                Err(engine::error::EngineError::UnsupportedAnsiSequence(seq)) => {
                    ttfx::errln!("Error: Unsupported ANSI sequence in input data: {seq:?}");
                    return ExitCode::from(1);
                }
                Err(e) => {
                    ttfx::errln!("Error: {e}");
                    return ExitCode::from(1);
                }
            };
        let mut effect = effect_command.build_effect();

        let outcome = if cli.parity_dump {
            ttfx::engine::effect::dump_effect(effect.as_mut(), &mut ctx, cli.max_frames)
                .map(|_| ttfx::engine::effect::RunOutcome::Complete)
        } else {
            ttfx::engine::effect::run_effect(effect.as_mut(), &mut ctx, tty_output)
        };
        match outcome {
            Ok(ttfx::engine::effect::RunOutcome::TerminalResized) => {
                // run_effect wiped the old area and left the cursor at its top,
                // so the rebuild lays out from here. --reuse-canvas would send
                // prep_canvas to a DEC anchor that no longer applies, so it only
                // governs the first run. Dropping this engine normally is what
                // keeps a long session of resizes from accumulating them.
                config.reuse_canvas = false;
                rng = ctx.rng;
            }
            done => {
                forget_engine(effect, ctx);
                break done.map(|_| ());
            }
        }
    };
    std::mem::forget(input_data);

    match result {
        Ok(()) => {
            if ttfx::terminated() {
                ttfx::die_from_sigterm();
            }
            if ttfx::interrupted() {
                ExitCode::from(1)
            } else {
                ExitCode::SUCCESS
            }
        }
        Err(e) => {
            ttfx::errln!("Error: {e}");
            ExitCode::from(1)
        }
    }
}

/// --music: random stock effects back to back, each on the music's clock.
fn run_music(cli: &cli::Cli, input_data: &str) -> ExitCode {
    use ttfx::engine::effect::RunOutcome;
    use ttfx::music::controls::{self, Change};
    use ttfx::music::source::{self, Source};

    if let Err(e) = controls::install() {
        ttfx::errln!("Error: cannot install music controls: {e}");
        return ExitCode::from(1);
    }
    let start = std::time::Instant::now();
    let spec = cli.music.as_deref().unwrap_or("live");
    let (source, delay_ms) = if spec == "live" {
        let sink = cli.music_sink.clone().or_else(source::default_sink);
        let delay = cli
            .music_delay
            .unwrap_or_else(|| source::saved_sync_ms(sink.as_deref()));
        match source::Live::start(cli.music_sink.as_deref()) {
            Ok(live) => (Source::Live(live), delay),
            Err(e) => {
                ttfx::errln!("Error: cannot start pw-record: {e}");
                return ExitCode::from(1);
            }
        }
    } else {
        match source::Fixture::open(std::path::Path::new(spec), cli.music_offset) {
            Ok(fixture) => (Source::Fixture(fixture), cli.music_delay.unwrap_or(0)),
            Err(e) => {
                ttfx::errln!("Error: cannot read music fixture {spec}: {e}");
                return ExitCode::from(1);
            }
        }
    };
    let start = match &source {
        Source::Live(live) => live.start_instant(),
        Source::Fixture(_) => start,
    };
    let mut music = ttfx::music::Music::new(source, delay_ms);
    music.stock = cli.music_stock;
    let mut config = cli.terminal_config();
    if config.frame_rate == 0 {
        config.frame_rate = 120;
    }
    let mut pace = ttfx::fx::run::Pace::new(
        config.frame_rate,
        cli.virtual_clock,
        start,
        cli.music_seconds,
    );
    let mut log = match &cli.music_log {
        Some(path) => match ttfx::fx::run::MusicLog::create(path) {
            Ok(log) => Some(log),
            Err(e) => {
                ttfx::errln!("Error: cannot write {}: {e}", path.display());
                return ExitCode::from(1);
            }
        },
        None => None,
    };
    let mut rng = match cli.seed {
        Some(seed) => ttfx::utils::rng::Rng::seeded(seed),
        None => ttfx::utils::rng::Rng::from_entropy(),
    };
    use clap::CommandFactory;
    let mut names: Vec<String> = cli::Cli::command()
        .get_subcommands()
        .map(|c| c.get_name().to_string())
        .collect();
    if !cli.include_effects.is_empty() {
        names.retain(|n| cli.include_effects.contains(n));
    }
    names.retain(|n| !cli.exclude_effects.contains(n));
    // clap's generated `help` subcommand is not an effect; never offer it up.
    names.retain(|n| is_effect_name(n));
    if names.is_empty() {
        ttfx::errln!("Error: No effects available after filtering.");
        return ExitCode::from(1);
    }
    // Each effect's stock length (seconds at 120 fps, 137x33): music speeds
    // effects up, so the shortest come up less often instead of flashing past.
    const STOCK_SECONDS: &[(&str, f64)] = &[
        ("overflow", 0.5), ("expand", 1.0), ("highlight", 1.1), ("middleout", 1.1),
        ("wipe", 1.1), ("slice", 1.4), ("slide", 1.4), ("sweep", 1.8), ("scattered", 2.0),
        ("randomsequence", 2.2), ("smoke", 2.3), ("rain", 3.0), ("unstable", 3.0),
        ("spray", 3.6), ("errorcorrect", 3.7), ("crumble", 3.9), ("pour", 4.0),
        ("burn", 4.4), ("colorshift", 4.4), ("waves", 4.6), ("beams", 4.8),
        ("synthgrid", 4.9), ("bouncyballs", 5.6), ("vhstape", 5.7), ("spotlights", 6.2),
    ];
    // Gentle effects (text appearing in place) suit calm music; the rest
    // move across the screen and suit driving music. The choice leans toward
    // what the music is doing now, without leaving anything out.
    const CALM: &[&str] = &[
        "print", "decrypt", "colorshift", "highlight", "errorcorrect", "laseretch",
        "spotlights", "orbittingvolley", "bubbles", "crumble", "randomsequence",
        "expand", "middleout", "wipe",
    ];
    let candidates: Vec<ttfx::music::rotation::Candidate> = names
        .iter()
        .map(|n| {
            let seconds = STOCK_SECONDS
                .iter()
                .find(|(name, _)| name == n)
                .map_or(6.0, |&(_, s)| s);
            ttfx::music::rotation::Candidate {
                name: n.clone(),
                weight: (seconds / 6.0).clamp(0.2, 1.0),
                calm: CALM.contains(&n.as_str()),
            }
        })
        .collect();
    let mut rotation = ttfx::music::rotation::Rotation::new(candidates);
    let mut playback = ttfx::music::rotation::Playback::default();
    let mut change = Change::Next;
    let tty_output = std::io::stdout().is_terminal();
    ttfx::install_sigint_handler();
    if tty_output {
        ttfx::install_sigterm_handler();
        ttfx::install_sigwinch_handler();
    }

    let mut resized = false;
    let mut warned_all_hidden = false;
    loop {
        if pace.finished() {
            break;
        }
        if !cli.music_ignore_preferences {
            reload_preferences(&mut rotation, &mut warned_all_hidden);
        }
        let intensity = music.intensity() as f64;
        let Some(name) = playback.select(&mut rotation, change, intensity, &mut rng) else {
            break;
        };
        // Stock settings, except that the two longest effects use their own
        // options to keep to the length of the others (matrix rains for 15
        // seconds and swarm flies ten flocks by default).
        let mut args = vec!["ttfx", name.as_str()];
        match name.as_str() {
            "matrix" => args.extend(["--rain-time", "6"]),
            "swarm" => args.extend(["--swarm-size", "0.25"]),
            _ => {}
        }
        let command = match clap::Parser::try_parse_from::<_, &str>(args) {
            Ok(cli::Cli {
                effect: Some(effect),
                ..
            }) => effect,
            _ => continue,
        };
        let Some(mut effect) = ttfx::fx::effects::build(&command) else {
            continue;
        };
        let mut run_config = config.clone();
        if resized {
            run_config.reuse_canvas = false;
            resized = false;
        }
        let clock = ttfx::engine::ctx::Clock::virtual_with_frame_rate(config.frame_rate);
        let mut engine = match ttfx::fx::Engine::new(input_data, run_config, rng, clock) {
            Ok(engine) => engine,
            Err(e) => {
                ttfx::errln!("Error: {e}");
                return ExitCode::from(1);
            }
        };
        let outcome = ttfx::fx::run::run_effect_music(
            effect.as_mut(),
            &mut engine,
            tty_output,
            &name,
            &mut music,
            &mut pace,
            &mut log,
        );
        rng = engine.rng;
        match outcome {
            Ok(RunOutcome::Complete) => {
                // The finished logo holds until a hit, and the next effect
                // starts on it (its first tick gets that accent). A pause
                // holds; music without clear hits, or no music for a while,
                // moves on after a moment.
                let held_from = pace.now();
                while !music.stock
                    && !pace.finished()
                    && !ttfx::interrupted()
                    && !ttfx::terminated()
                    && !controls::pending()
                {
                    music.advance(pace.now(), ttfx::music::MAX_TICKS_PER_FRAME);
                    if music.has_accent()
                        || (pace.now() - held_from > 1.2
                            && (music.frame.presence > 0.5 || music.idling()))
                    {
                        break;
                    }
                    if let Some(log) = &mut log {
                        log.line(&pace, "hold", &music.frame, 0);
                    }
                    pace.wait();
                }
            }
            Ok(RunOutcome::TerminalResized) => resized = true,
            Ok(RunOutcome::Interrupted) => return ExitCode::from(1),
            Ok(RunOutcome::Terminated) => {
                drop(log);
                ttfx::die_from_sigterm();
            }
            Ok(RunOutcome::OutputClosed) => return ExitCode::SUCCESS,
            Err(e) => {
                ttfx::errln!("Error: {e}");
                return ExitCode::from(1);
            }
        }
        change = controls::take().unwrap_or(Change::Next);
    }
    ExitCode::SUCCESS
}

/// Whether a clap subcommand name builds an effect. Filters the generated
/// `help` subcommand (and any future non-effect) out of the rotation.
fn is_effect_name(name: &str) -> bool {
    matches!(
        clap::Parser::try_parse_from::<_, &str>(["ttfx", name]),
        Ok(cli::Cli {
            effect: Some(_),
            ..
        })
    )
}

/// Re-read the shared preferences before each effect, so an edit takes effect
/// on the next selection rather than per frame. An unusable file or an
/// all-hidden set is reported and the last eligible set is kept.
fn reload_preferences(
    rotation: &mut ttfx::music::rotation::Rotation,
    warned_all_hidden: &mut bool,
) {
    use ttfx::music::rotation::{Preferences, PrefsOutcome};
    match Preferences::load_default() {
        Ok(prefs) => match rotation.apply_preferences(&prefs) {
            PrefsOutcome::Applied => *warned_all_hidden = false,
            PrefsOutcome::EverythingHidden => {
                if !*warned_all_hidden {
                    ttfx::errln!(
                        "Warning: every effect is hidden in effects.conf; keeping the last eligible set."
                    );
                    *warned_all_hidden = true;
                }
            }
        },
        Err(e) => ttfx::errln!("Warning: ignoring effects.conf: {e}"),
    }
}

/// M0 parity path: build the Terminal, make every character in
/// character_by_input_coord visible, print the first frame to stdout.
fn m0_dump(input_data: &str, cli: &cli::Cli) -> ExitCode {
    let config = cli.terminal_config();
    let mut terminal = match Terminal::new(input_data, config) {
        Ok(t) => t,
        Err(engine::error::EngineError::UnsupportedAnsiSequence(seq)) => {
            ttfx::errln!("Error: Unsupported ANSI sequence in input data: {seq:?}");
            return ExitCode::from(1);
        }
        Err(e) => {
            ttfx::errln!("Error: {e}");
            return ExitCode::from(1);
        }
    };
    let ids: Vec<_> = terminal
        .character_by_input_coord
        .values()
        .copied()
        .collect();
    for id in ids {
        terminal.set_character_visibility(id, true);
    }
    print!("{}", terminal.get_formatted_output_string());
    println!();
    ExitCode::SUCCESS
}
