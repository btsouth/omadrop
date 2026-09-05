# projectM runtime

Omadrop bundles dynamically linked projectM 4.1.7 with a private eight-value
per-instance audio input bridge. Source revision and submodule are pinned by
`bin/fetch-projectm`. Run `bin/build-milkdrop-runtime` to fetch, patch and build.
The full patch recipe is in `experiments/milkdrop-audio-pilot/prepare.py`.
Original PCM, FFT and preset waveforms remain enabled.

`LICENSE.txt` covers projectM. `projectm-eval-LICENSE.md` and
`hlslparser-LICENSE` cover its bundled MIT dependencies. No system library is
replaced. The application searches its private `lib` directory.
