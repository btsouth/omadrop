# Journey study

An original Osaka Jade city to Kanagawa sea prototype, rendered by
`Journey::paint` in both a native fullscreen Qt Quick window and headless JPEG
export. This is a study, not accepted artwork or a product integration.

Requires Qt 6 Core/Gui/Qml/Quick, qmake6, a C++17 compiler and FFTW single
precision (`fftw3f`). Build and export on devbox; run native previews only inside
an isolated omabox desktop. No installation is needed.

Build from this directory on devbox:

```sh
mkdir -p build
cd build
qmake6 ../journey.pro
make -j4
```

Native preview inside omabox:

```sh
build/omadrop-journey --audio /path/song.f32 --audio-offset 12 --duration 72
build/omadrop-journey --audio /path/song.f32 --audio-offset 12 --at 36 --paused
```

| Key | Action |
| --- | --- |
| Esc / Q | Quit |
| Space | Pause / resume |
| Left / Right | Scrub backward / forward one movie second |

Playback stops at the requested duration. `--at` starts at a movie second;
`--paused` gives a deterministic native still. Movie time is passed directly to
the scene, without changing its pacing. Width/height default to 1920x1080 for
export; native fullscreen follows the compositor's output size.

Headless export on devbox:

```sh
build/omadrop-journey --audio /path/song.f32 --audio-offset 12 \
  --export-dir /path/frames --fps 30 --duration 72 --width 1920 --height 1080
```

This constructs `QCoreApplication`, never `QGuiApplication`, and paints to
`QImage` with the same scene function. Keep that function free of fonts/text
and GUI resources. Export writes JPEG quality 94, named `frame-000000.jpg`
onward. Frame `i` samples movie time `i / fps`; the endpoint is excluded. Existing
target frames cause failure before rendering unless `--overwrite` is explicit.
Unrelated files in the directory are retained. A later write failure can leave
earlier completed frames. `--at` and `--paused` are native-only options.

Audio input is headerless interleaved stereo 44100 Hz float32 little-endian PCM,
not WAV. For example, prepare a private fixture with:

```sh
ffmpeg -i /path/song.wav -ar 44100 -ac 2 -f f32le /path/song.f32
```

The harness reads the fixture from sample zero to `audio-offset + duration`,
warming all analyzer and smoothing history before the requested offset. It
precomputes at 60 Hz and interpolates at exact movie time plus offset, including
fractional offsets. Six frequency ranges are 25..70, 70..150, 150..400,
400..1500, 1500..4000 and 4000..12000 Hz. These are frequency roles, not isolated
instruments. Bands share an absolute-power mapping and attack/release smoothing;
each role retains its measured level instead of being normalized to its own
peak. The existing read-only `projectm-ascii/audio_features.h` analyzes each
channel independently for bass body and onset accent. Bass combines by RMS
and onsets by maximum, preserving opposite-phase stereo. A supplementary
stereo FFT supplies absolute bands.

Omitting `--audio` deliberately renders silence. The harness analyzes recorded
audio but does not play it. Sound can be muxed into exported frames separately.
Invalid paths, incomplete stereo frames, nonfinite samples, magnitude above 16,
and recordings too short for the whole offset + duration fail with an error.
Samples beyond the required range are not analyzed. Numeric inputs must be
finite: duration 1/60..600 seconds, fps 1..120, dimensions 1..8192 with at most
33554432 pixels, and offset + duration at most six hours.

Focused audio checks need no window or scene implementation. Build/run on devbox:

```sh
mkdir -p build-tests
cd build-tests
qmake6 ../tests/audio_tests.pro
make -j4
./journey-audio-tests
```

The checks cover warmed fractional offsets, interpolation, role balance,
opposite-phase stereo, silence and malformed/short input. They establish harness behavior, not beauty
or musical acceptance. No live PipeWire, product backend, website, installer or
desktop configuration is changed by this study.
