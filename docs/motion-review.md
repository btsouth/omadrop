# Measuring musical movement

User feedback: repetitive zooming/twitching, too little motion in places, and
insufficient visual variety. Large local transformations are welcome when they
follow the sound. A named song and passage are pending for targeted listening.
Do not add random motion, a beat-driven camera, or automatic movement to raise
a motion score.

## Repeatable review

`bin/omadrop-motion-review VIDEO.mp4 OUTPUT_DIRECTORY` creates a local HTML
review with synchronized playback, seekable audio/visual traces, flagged moments,
and `measurements.json`. It requires NumPy and FFmpeg. It only decodes the file;
it does not play audio, capture microphone/system audio, or change audio routing.
The report opens paused. Existing output directories are rejected to preserve
before/after evidence. This is a developer diagnostic, not a new runtime dependency.

The movie must contain its matching audio. Use the same source passage, duration,
resolution, and color pipeline for comparisons. For example:

```sh
OMADROP_VIDEO_START_SECONDS=26 bin/native-song-video \
  cache/rights-replay/beat-me.f32 cache/example.mp4 constellation-field 38
bin/omadrop-motion-review cache/example.mp4 cache/example-review
bin/omadrop-motion-review-test
```

The analyzer decodes 30 FPS grayscale frames at 160x90 and independent 48 kHz
mono audio. Frequency measurements use a two-frame Hann window. It does not
consume Omadrop's beat labels, avoiding a test that merely repeats the engine's
own predictions. Lag estimates have 33 ms resolution plus window/decoding
uncertainty. They are correlations, not proof of perception or causation.

## What the measurements mean

- **Pixel change:** magnitude of luminance change between frames. This includes
  lighting and motion. More is not automatically better.
- **Radial-scale explained fraction:** motion-weighted incremental fit from a
  centered radial expansion/contraction model, after fitting translation and
  brightness gain/offset. It is a zoom-like appearance proxy, not camera tracking
  or a literal fraction of moving objects. Nonlinear and off-center movements
  may be poorly represented.
- **Spatial effective rank:** diversity of change across a fixed 4x4 image grid.
  A value near one means most activity uses much the same spatial pattern.
  It loses subregion details and depends on scene composition.
- **Consecutive onset response similarity:** cosine similarity between regional
  changes after independent spectral-flux onset candidates. Similar successive
  drum hits may correctly produce similar responses. This is not a repetition
  defect detector or an instrument-separation score.
- **Flux/activity lag:** strongest correlation over roughly plus/minus 233 ms.
  A positive lag means picture activity follows the audio feature. The tool can
  miss sustained/pitch events and unrelated visual accents can correlate by chance.
- **Review flags:** relatively quiet picture response after an audio onset, or
  an unusually abrupt visual change. A person must decide whether the mismatch
  is real. These are not automatically classified as missed notes or false motion.

No combined grade is produced. Beauty, interest over ten minutes, correct musical
meaning, and an S-tier experience remain human judgments. Repetition in the
music is not a license to make up a different movement every time.

## Controlled comparison

Baseline recordings for the three current candidates are in
`cache/collection-batch1/`. Initial reports in `cache/motion-diagnosis/` found
Opal Bloom strongly explained by radial scaling, consistent with its old global
radius/thickness mappings. All three recordings showed very similar regional
responses at consecutive onset candidates. This supports investigating limited
movement vocabulary; it does not establish correctness for every song.

Opal's revised mapping uses opposing low-frequency petal deformation, a local
kick-induced depth buckle, middle-frequency fold opening, and frequency-specific
surface variation. The camera remains fixed. It preserves held-input stillness
and brief settling. The first trial removed most radial fit but also reduced
activity; a stronger local deformation is being compared rather than presenting
less movement as an unqualified improvement.

Next: use the user's named rap passage to annotate audible events, expected
visual targets, observed response, and unwanted repetition. Compare revisions
against that reference plus contrasting passages. Do not tune the whole engine
to a single short fixture or claim that these frequency bands isolate vocals.

## Installed local-buckling trial

Opal Bloom's stronger local deformation is installed. Comparing identical
12-second excerpts (26..38 seconds of the existing `beat-me.f32` fixture):

| Diagnostic | Baseline | Local folds |
| --- | ---: | ---: |
| Mean pixel change | 0.002682 | 0.002409 |
| Radial-scale explained fraction | 0.4107 | 0.0189 |
| Spatial effective rank | 1.131 | 1.307 |
| Consecutive onset response similarity | 0.994 | 0.984 |
| Flux/activity correlation at best lag | 0.480 | 0.538 |

Both lag estimates select 0 ms at this tool's coarse resolution. This is not
an end-to-end latency measurement. Regional repetition remains high; the change
addresses the zoom-like defect but does not establish sufficient variety or
user acceptance. Mean image activity is slightly lower even with stronger local
deformation, illustrating why pixel-change magnitude alone is insufficient.

Reports: `cache/motion-diagnosis/opal-baseline/index.html` and
`cache/motion-diagnosis/opal-local-folds/index.html`. JSON traces are beside each
report. The final report was rendered and visually inspected in an isolated
headless browser. Its video opens paused with a poster frame and separate
signal lanes; clicks on the chart or flags seek the synchronized recording.

Validation: analytic radial motion versus exposure/translation, lag direction,
static-image handling, and a decoded static/silent movie all pass. The Opal
shader's role, held-input, silence, and settling contracts pass; the installed
GPU probe reports `native_renderer=ok`. The source and installed shader match.
No live audio was captured for this work, and no system audio routing changed.
