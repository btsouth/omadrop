# Omarchy screensaver with music

Omarchy's screensaver is ttfx (the Rust TerminalTextEffects port) playing
random effects on the Omarchy logo in a fullscreen terminal. This is that
screensaver with Omadrop's audio engine brought into it. The effects, the
logo, the terminal and its settings are the real ones; nothing is redrawn
or re-created.

`ttfx/` is ttfx v0.5.0 (112ebb3, the version Omarchy installs) with:

- `src/music/`: Omadrop's `AudioFeatureBus` (projectm-ascii/audio_features.h)
  ported to Rust (same window, hop, bands, onset thresholds and beat clock),
  PipeWire capture of the default output as the MilkDrop app does it, and the
  per-output speaker delay that app saves.
- `src/fx/run.rs` `run_effect_music`: the music is the effects' clock. Each
  display frame gets as many stock ticks as the music grants. Paths, easing,
  scenes and colors play exactly as authored, at the music's pace:
  - silence freezes within 50 ms; more than 6 s of it (a stopped player)
    drifts at 0.35x like the plain screensaver;
  - while music plays, continuous progress has a 60 ticks/s floor (0.5x
    the launcher's 120 Hz stock rate). After 8 seconds of playing music,
    long effects gain finishing progress; paused time does not count;
  - strong kicks and snares grant a bounded stretch of authored movement,
    eased over 120 ms and spaced at least 280 ms apart. Routine hits nudge
    the clock, making stronger changes distinct during fast music;
  - music-clock rates use elapsed seconds, and surplus ticks survive the
    per-frame cap. Silence clears both pending motion and pending accents.
- Accents: each kick or snare is handed to the effect for one tick. 15
  effects hold a due launch for it instead of their frame countdown
  (fireworks shells, beam groups, lightning, ball drops, bubbles, crumbling,
  error swaps, volleys, ring spins, black hole explosion, swarm flights, VHS
  glitches, unstable's jolts, matrix columns, rain and spray), and 12 more
  release a burst of their own reveal step on it (pour, burn, decrypt typing,
  print, random sequence, laser etch, waves, binary paths, synth grid blocks,
  smoke, overflow, slide). Each is a few lines in the effect's `next_frame`,
  and a no-op without music.
- `--music live|FILE`: random effects back to back in one process; each
  starts on a hit. A shuffle bag plays every enabled effect once per round,
  avoiding the last eight choices across rounds when possible. Order is
  weighted by stock length (short effects get lower priority), and leans on the
  music's intensity: driving music favors the effects that move across the
  screen, calm music the gentle in-place ones. Nothing is left out. matrix and
  swarm use their own `--rain-time 6` and `--swarm-size 0.25`.

Without `--music` the binary is stock ttfx: all 37 effects produce
byte-identical output to upstream (checked with `--parity-dump`, two seeds
each).

These are signal measurements (band onsets, loudness), not instrument or
chorus recognition.

## Run

    ./install.sh          # build/ttfx-music must exist (see Build)
    omadrop-screensaver   # every monitor, like omarchy-launch-screensaver; any key exits

`bin/omadrop-screensaver` follows Omarchy's terminal/screensaver setup with
`bin/omadrop-screensaver-run` in place of `omarchy-screensaver`. Its own class
(`org.omadrop.screensaver`) and fullscreen launch rules keep its lifecycle
separate from stock Omarchy. `--single`/`--all` share Omadrop's display choice.

For the shared product command and visual mode chooser, see
[`product/README.md`](product/README.md). MilkDrop remains the accepted installed
backend; Omarchy preserves actual ttfx characters. No Originals mode is enabled.

## Build

    ssh devbox-bts 'cd /workspace/worktrees/omadrop-omarchy-flagship/experiments/screensaver-music/ttfx && PATH="$HOME/.cargo/bin:$PATH" cargo build --release --locked'
    rsync -a devbox-bts:/workspace/worktrees/omadrop-omarchy-flagship/experiments/screensaver-music/ttfx/target/release/ttfx build/ttfx-music

## Review tools (`review/`)

- `render-run.sh`: an offline run from a fixture at the real screensaver's
  grid (Ghostty, font size 18, 1920x1080 = 137x33): exact terminal bytes plus
  a per-frame log (tempo, ticks, onsets, rewritten cells, byte offsets).
- `box-capture.sh`: replays those bytes frame by frame into real Ghostty in
  an omabox box, captures 60 fps and muxes the fixture audio. Frame-exact
  sync without a screen recorder.
- `lockin.py`, `per_effect.py`, `pixel_motion.py`: visible change around the
  reference track's hits, compared against stock-clock, shifted, different
  and silent runs of the same seed.
- `OMADROP_MUSIC_TUNING="kick_push=7.5,flow_gain=1.2,..."` overrides the
  conductor (`Tuning` in `src/music/mod.rs`) for experiments.


## Browse and curate effects

Run `omadrop --effects`, or open **Omadrop Effects** from the launcher. All 37
actual ttfx effects are listed with descriptions. Preview any effect, mark
favorites, hide effects you dislike, or start the rotation. Favorites receive
priority in the order; every enabled effect still gets one turn each round.

Curation lives separately in `$XDG_CONFIG_HOME/omadrop/effects.conf`:

    version=1
    favorites=fireworks,swarm
    hidden=burn,pour

Changes apply at the next effect. A selected preview ignores hidden preferences,
repeats that effect until any key exits, and returns to the browser. Previewing
closes the current Omadrop session but preserves saved mode and display choices.
`omadrop --effect beams` launches a selected effect and remembers Omarchy mode.

V3 music-playing tuning is retained. One silent-clock bug was fixed: fractional
idle ticks now accumulate rather than being discarded, so silent previews move
and long silence returns to drift as intended. A four-second pause stays still.
See `review/rotation-browser-review.md` for checks and installed build identity.
