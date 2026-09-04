# Silent visual-legibility testing

Omadrop does not claim to make music accessible to deaf or hard-of-hearing
people without direct testing. Renderer metrics prove that cues exist and that
visual policies preserve them. Only people watching the visuals can establish
whether the cues are understandable, comfortable, and worth using.

## Build a study packet

Run the normal build, then use one of the exact audio files in
[`demo/music-rights.json`](../demo/music-rights.json):

```bash
experiments/projectm-ascii/build.sh
bin/omadrop-accessibility-study /path/to/approved.mp3 /tmp/omadrop-study
```

The generator verifies the music rights record, uses the production analyzer
to select an 18-second excerpt containing a section change, and renders nine
anonymous, silent clips from that exact window. Ink Current, Shadow
Architecture, and Particle Weave each appear once with the default policy,
reduced motion, and the flash limit. The order separates matching scenes so
participants are not told which scene or policy they are viewing. Pass a whole
number of seconds as the third argument only when testing a specific window;
the generator rejects a window without a detected section change.

Open `/tmp/omadrop-study/index.html` in a browser. During each clip, the viewer
marks perceived beats with Space and larger musical changes with Enter, then
rates beat clarity, role separation, structure, comfort, and beauty. The page
has no network requests and collects no name, hearing status, microphone data,
or browser history. It writes nothing until the participant explicitly
downloads an anonymous JSON result.

Keep `study.json` and `truth/` with the facilitator until the participant has
finished. They reveal the scene, visual policy, and analyzer timeline behind
each anonymous clip.

## Score results locally

Pass one or more downloaded result files to the scorer:

```bash
bin/omadrop-accessibility-score /tmp/omadrop-study results/*.json \
  > /tmp/omadrop-study/report.md
```

The report measures beat marks within 180 ms and section marks within 1.5
seconds, then reports ratings by scene and policy. It never uploads results.

## Review standard

Treat the first sessions as product research, not certification. Before making
public accessibility claims:

1. Include at least five deaf or hard-of-hearing participants recruited with
   informed consent, plus hearing participants only as a comparison group.
2. Review individual results rather than hiding a poor scene behind an average.
3. Require reduced-motion and flash-limited variants to retain the timing
   clarity of the matching default scene while improving or preserving comfort.
4. Investigate repeated beat precision below 70 percent, median matched-beat
   error above 180 ms, or average clarity, comfort, or beauty below 4 out of 5.
5. Hold a short follow-up conversation about which visual changes actually
   meant kick, backbeat, high percussion, and section movement to the viewer.
6. Change mappings that participants cannot explain consistently, then repeat
   the same blinded study with a new packet.

These thresholds are product-quality signals, not medical or accessibility
certification. Keep public wording factual until the target audience has
completed the test.
