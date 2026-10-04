# Osaka Jade renderer

Omarchy mode's living world. The package installs it as `bin/omadrop-osaka`,
and it plays on every Omarchy theme. It contains the Osaka street, the vector
canvas and compositor, the character rigs and the sign outlines.

Build with `qmake6 ../osaka.pro && make` from a `build` directory. Esc closes
all Osaka windows and returns to the controls.

## Music

Audio comes from the default output through PipeWire at 44100 Hz stereo, in
735-frame hops. Quiet playback is lifted to a normal listening level before
analysis, so the street reacts the same at low volume; it is never turned down,
and silence stays silent. The analyser measures six frequency bands, kicks,
onsets, mid peaks and swells as they happen. Nothing looks ahead in the music.

## Timeline

Each launch gets its own seeded timeline. Trains, cyclists, gusts, tea,
cooking, toasts and small sky moments recur on separate schedules, and
gestures and windows have their own periods. A strong kick, onset or swell can
start fireworks, at most once every 45 to 90 seconds. Songs with a strong bass
body get the full show with follow-up shells; quieter songs get one small
shell. Birds scatter, leave and come back on their own. The street keeps
moving through silence.

`--single` plays on the focused monitor and `--all` on every monitor, sharing
the MilkDrop display preference. Slower GPUs lower the internal resolution
automatically; `OMADROP_OSAKA_SCALE` (0.5 to 1) fixes it.

## Tools and tests

- `--probe` measures the music response without drawing.
- `--bench` measures drawing without encoding.
- `--record out.mp4` records a live session in real time, 60 seconds at 1080p30
  by default (`--fps 60` and `--width`/`--height` are supported).
- `--stats out.csv` writes per-frame response and timing.
- `--verify-render` checks that the optimized canvas matches the reference
  output exactly on three poses, including fireworks.
- `OMADROP_PW_RECORD_COMMAND` can feed recorded PCM instead of system audio.

The CMake tests in `tests/` cover streaming analysis, chunk boundaries,
quiet playback, silence, eight simulated hours of bounded state, firework
cooldowns and the canvas geometry.
