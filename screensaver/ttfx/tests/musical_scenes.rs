//! Native geometry and lifecycle regressions; headless, no audio or desktop.
use clap::Parser;
use ttfx::cli::Cli;
use ttfx::engine::ctx::Clock;
use ttfx::engine::terminal::TerminalConfig;
use ttfx::fx::run::Effect;
use ttfx::fx::Engine;
use ttfx::music::Cue;
use ttfx::utils::rng::Rng;

fn setup(name: &str) -> (Box<dyn Effect>, Engine) {
    let cli = Cli::try_parse_from(["ttfx", name]).unwrap();
    let effect = ttfx::fx::effects::build(&cli.effect.unwrap()).unwrap();
    let config = TerminalConfig {
        frame_rate: 120,
        canvas_width: 45,
        canvas_height: 15,
        ignore_terminal_dimensions: true,
        ..TerminalConfig::default()
    };
    let engine = Engine::new(
        "OMARCHY MUSIC\n###  ###  ###\n###  ###  ###\nOMADROP SCENE",
        config,
        Rng::seeded(16),
        Clock::virtual_with_frame_rate(120),
    )
    .unwrap();
    (effect, engine)
}

#[test]
fn new_controls_do_not_change_stock_frames_or_completion() {
    for name in ["rings", "beams"] {
        let (mut stock, mut a) = setup(name);
        let (mut extended, mut b) = setup(name);
        stock.build(&mut a).unwrap();
        extended.build(&mut b).unwrap();
        let mut complete = false;
        for tick in 0..12_000 {
            b.cue = Cue {
                bands: [1.0; 6],
                bass_body: 1.0,
                scene_age: tick as f64,
                ..Cue::default()
            };
            let next_a = stock.next_frame(&mut a);
            let next_b = extended.next_frame(&mut b);
            assert_eq!(next_a, next_b, "{name} completion at {tick}");
            assert_eq!(a.ch.coord, b.ch.coord, "{name} geometry at {tick}");
            assert_eq!(a.ch.visual, b.ch.visual, "{name} visuals at {tick}");
            assert_eq!(a.ch.flags, b.ch.flags, "{name} visibility at {tick}");
            if !next_a {
                complete = true;
                break;
            }
        }
        assert!(complete, "{name} stock lifecycle should finish");
    }
}

#[test]
fn sustained_effects_keep_moving_without_onsets_then_finish() {
    for name in ["rings", "beams"] {
        let (mut effect, mut e) = setup(name);
        effect.build(&mut e).unwrap();
        let mut snapshots = Vec::new();
        for tick in 0..(27 * 120) {
            e.cue = Cue {
                active: true,
                scene: true,
                scene_age: tick as f64 / 120.0,
                presence: 1.0,
                bands: [0.5; 6],
                bass_body: 0.2,
                ..Cue::default()
            };
            assert!(
                effect.next_frame(&mut e),
                "{name} ended at {}",
                e.cue.scene_age
            );
            if tick >= 20 * 120 && tick % 41 == 0 {
                snapshots.push((e.ch.coord.clone(), e.ch.visual.clone()));
            }
        }
        assert!(
            snapshots.windows(2).any(|s| s[0] != s[1]),
            "{name} became static"
        );
        // Silent fallback still departs at the deadline and drains/finishes.
        let mut ended = false;
        for tick in 27 * 120..=34 * 120 {
            e.cue = Cue {
                active: true,
                scene: true,
                scene_age: tick as f64 / 120.0,
                ..Cue::default()
            };
            if !effect.next_frame(&mut e) {
                ended = true;
                break;
            }
        }
        assert!(ended, "{name} transition exceeded the deadline");
    }
}

#[test]
fn rings_bass_changes_native_geometry_at_identical_clock_and_seed() {
    let (mut quiet, mut a) = setup("rings");
    let (mut bass, mut b) = setup("rings");
    quiet.build(&mut a).unwrap();
    bass.build(&mut b).unwrap();
    for tick in 0..1_800 {
        a.cue = Cue {
            active: true,
            scene: true,
            scene_age: tick as f64 / 120.0,
            bands: [0.4; 6],
            ..Cue::default()
        };
        b.cue = Cue {
            bass_body: 0.3,
            ..a.cue
        };
        assert!(quiet.next_frame(&mut a));
        assert!(bass.next_frame(&mut b));
    }
    let extent = |e: &Engine| -> i64 {
        e.input_chars
            .iter()
            .filter(|&&slot| e.is_visible(slot))
            .map(|&slot| (e.coord(slot).column - e.canvas.center.column).abs())
            .sum()
    };
    assert!(
        extent(&b) > extent(&a) * 12 / 10,
        "bass should expand actual orbiting characters"
    );
}

#[test]
fn prototype_requires_explicit_music_and_defaults_off() {
    assert!(Cli::try_parse_from(["ttfx", "--music-scenes"]).is_err());
    assert!(!Cli::try_parse_from(["ttfx", "rings"]).unwrap().music_scenes);
    assert!(
        Cli::try_parse_from(["ttfx", "--music", "fixture.f32", "--music-scenes"])
            .unwrap()
            .music_scenes
    );
}

#[test]
fn sustained_cli_rejects_rates_that_cannot_deliver_its_native_clock() {
    for rate in ["5", "-1"] {
        let output = std::process::Command::new(env!("CARGO_BIN_EXE_ttfx"))
            .args(["--music", "live", "--music-scenes", "--frame-rate", rate])
            .output()
            .unwrap();
        assert!(!output.status.success());
        let message = String::from_utf8_lossy(&output.stderr);
        // clap can reject negative values before the prototype validation.
        assert!(message.contains("at least 30") || message.contains("unexpected argument"));
    }
}
