# Collection engine and preset adaptations

This directory contains the generator for Omadrop's 21-preset collection.
The historical directory name is retained; the normal application now uses it.

`bin/build-milkdrop-runtime` fetches pinned projectM 4.1.7 source and its eval
submodule, applies the private per-instance bridge, generates the adaptations,
and compiles the player. `audio_controls.h` retains revision 6's stereo band
and timbral analysis. Native projectM PCM, FFT and waveform inputs remain.

Each preset maps low, mid, high, tone, air and energy to authored behaviors.
Frequency ranges are not separated instruments. The exposed attack value is
currently unused. `prepare.py` verifies original hashes before substitution.

`audio_controls_test.cpp` tests stereo independence, band selectivity, gating,
dynamics, decay and finite bounds. `bridge_test.cpp` verifies actual GL output,
instance isolation and bounded equation inputs. The release suite also loads
all 21 presets with real music and checks paired display controls.

`build.sh`, `install.sh` and `run.sh` retain the separate experimental launcher.
For the public application's build/install and supported controls, use the root
README and `docs/releasing.md`.
