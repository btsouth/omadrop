# Renderer fidelity check

Build `experiments/osaka-live/osaka.pro`. `--fidelity DIR --fixture FILE` runs
surfaceless EGL with `QCoreApplication`, without a desktop or live capture.
`FILE` must be stereo native-endian float32 PCM at 44100 Hz, in whole 735-frame
hops. Use identical fixture bytes, dimensions, seed, GPU and driver for both
builds. Supply `--seed 1 --width 1920 --height 1080` explicitly.

The mode first saves the existing synthetic `--verify-render` poses at 8, 14
and 28 seconds. It then consumes PCM through the production streaming analyzer,
score and schedule, using exact hop timestamps. It renders at a fixed 5 Hz and
records every frame's mean Rec.709 brightness and mean absolute per-pixel
brightness change in `frames.csv`. It saves PNGs every two seconds, plus 22, 26,
42 and 54 seconds. Compare quiet and active full-show frames using the manifest's
firework timestamp and flag: active means `0 <= seconds - firework_at < 12`.
The capture is bounded by fixture length, up to ten minutes. Internal scale is
fixed at 1.0; normal live and benchmark behavior is unchanged.

Run each build with an external timeout, then compare:

```sh
timeout 120s BASELINE --fidelity /tmp/baseline --fixture /path/fixture.f32 --seed 1 --width 1920 --height 1080
timeout 120s CANDIDATE --fidelity /tmp/candidate --fixture /path/fixture.f32 --seed 1 --width 1920 --height 1080
python3 experiments/osaka-live/tools/compare-frames.py /tmp/baseline /tmp/candidate /tmp/comparison
```

The Python comparator requires numpy and Pillow. It rejects mismatched capture
sets, dimensions and schedule manifests. `metrics.json` reports max and mean
absolute RGB channel differences in byte units, percent of pixels with any
channel differing by more than 2 or 8, and RGB PSNR in dB. Identical PSNR is the
string `infinity`. Each capture has a side-by-side PNG (baseline left) and an
absolute difference PNG amplified 16 times. The reactivity diagnostic averages
consecutive frame brightness change across the full fixture, omitting the first
frame. This is a fixed 5 Hz regression diagnostic, not an artistic score or proof
of instrument recognition. Numerical metrics support visual inspection; they
cannot approve a visible rendering change.

Keep private music, captures and machine-specific benchmark logs outside the
repository. Record source commits, fixture SHA-256, GPU/driver output and the
exact commands alongside the comparison.
