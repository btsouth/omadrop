# Omarchy mode

Omadrop uses the 37 terminal effects from ttfx, the Rust port of
TerminalTextEffects used by the Omarchy screensaver. Music controls their pace
and accents through local PipeWire capture.

The source is in `ttfx/`, based on upstream v0.5.0. Fork details and credits are
in `ttfx/OMADROP-PROVENANCE.md`, `ttfx/LICENSE` and `ttfx/NOTICE`.

Open Omadrop, select Omarchy, and press Play or click an effect card. The
controls offer search, favorite, hide and preview. Favorites receive priority;
every enabled effect gets a turn per round. Preferences live in
`~/.config/omadrop/effects.conf`. A selected preview can show a hidden effect.
Esc returns to the controls; Q and focus loss also end playback.

`bin/omadrop-screensaver` starts fullscreen terminals with
`bin/omadrop-screensaver-run`. That helper runs the installed `ttfx-music`
binary against the user's Omarchy branding text. `--single` and `--all` share
the product's display choice. The renderer uses the saved output timing.

```sh
(cd screensaver/ttfx && cargo build --release --locked)
(cd screensaver/ttfx && cargo test --locked)
```

The installer stages the resulting binary with the controls and MilkDrop.
See [building](../docs/building.md) for package and headless checks, and
[controls](../docs/controls.md) for supported keys.

For development review, `ttfx --music SOURCE --music-scenes` selects only
Rings and Beams. This prototype is opt-in; the installed helper and stock
nonmusic effects retain their existing behavior. `--include-effects rings` or
`--include-effects beams` isolates one scene, and `--music-stock` disables both
the new geometry and the music clock for an authored stock comparison.

Rings keeps the native alternating orbits alive. Smoothed bass body changes
formation extent; sustained middle and high band activity changes alternating
rings' aspect. Accents launch a brief native dispersal and regrouping, at most
once every three seconds. Beams keeps the native row/column sweeps crossing a
dim resting logo. Middle and high activity populate their own sweep direction,
bass widens adjacent beam clusters, and accents launch a crossing formation.
Symbols, gradients and native paths stay with their effects. These are frequency
and onset estimates, not instrument identification.

The prototype requires a display frame rate of at least 30 Hz; `--frame-rate 0`
selects 120 Hz. It runs the native paths at a steady 120 ticks per second instead
of accelerating their timeline. Quiet passages relax the formation and coverage;
silence clears measured activity and accents while sparse native motion remains.
Each scene sustains until at least 28 seconds, starts its authored home/wipe
transition on an accent or by 30 seconds, and finishes by 34 seconds. The next
scene still uses the existing short logo hold. Very large canvases may reach the
four-second transition deadline before the home animation completes. This is a
review candidate; listening and aesthetic acceptance remain open.

Targeted headless checks (run builds and tests on the development machine):

```sh
cd screensaver/ttfx
cargo test --locked music::scenes
cargo test --locked music::tests
cargo test --locked --test musical_scenes
cargo build --release --locked
```

Fixture review uses interleaved stereo f32 at 44.1 kHz and the existing ANSI
review tooling. Run this in a scratch build tree, with your branding input and
actual music fixture copied there. Review output belongs outside the repository.

```sh
target/release/ttfx -i /path/to/screensaver.txt \
  --frame-rate 120 --canvas-width 137 --canvas-height 33 \
  --ignore-terminal-dimensions --reuse-canvas --anchor-canvas c --anchor-text c \
  --no-eol --no-restore-cursor --seed 16 --virtual-clock \
  --music /path/to/preview.f32 --music-scenes --music-ignore-preferences \
  --music-offset 54 --music-seconds 36 --music-log /path/to/rings.csv \
  --include-effects rings > /path/to/rings.ans
```

Repeat with `beams`, then without `--include-effects` for a 70-second two-scene
rotation. Repeat the same seed and audio offset without `--music-scenes` for the
prior music-paced choreography, and with `--music-stock` for stock playback.
CSV appends `scene`, `scene_age`, `bass_body` and the six continuous bands; the
existing columns, including ANSI byte boundaries, keep their names and order.
