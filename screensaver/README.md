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
