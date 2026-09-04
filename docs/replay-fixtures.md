# Native replay fixtures

Omadrop's baseline replay audio is synthesized by
`experiments/projectm-ascii/scorecard_fixture.cpp`. It contains no samples,
recordings, stems, generated music, or third-party compositions. The fixture
source and every waveform it produces are covered by the repository license.

The suite is deliberately diagnostic. It is not release music and does not
replace review with real, rights-cleared songs.

| Profile | Duration | Primary stress case |
| --- | ---: | --- |
| `structured-electronic` | 32 seconds | Quiet intro, breakdown, build, peak, and release |
| `sparse-acoustic` | 24 seconds | Low-level plucks, brush-like hits, negative space, and isolated transients |
| `dense-compressed` | 24 seconds | Limited dynamics, rapid hats, overlapping percussion, and a short breakdown |
| `sustained-vocal` | 24 seconds | Long tonal envelopes, vibrato, phrase gaps, and sparse percussion |
| `syncopated-sections` | 24 seconds | Irregular 16-step patterns, stereo alternation, and fast section changes |

Generate and verify the locked set with:

```sh
./bin/native-replay-fixtures --verify
```

The expected SHA-256 values are stored in
`experiments/projectm-ascii/fixtures/replay-suite.sha256`. A changed waveform
requires an intentional fixture update, a complete scorecard-suite run, and a
reviewed manifest change.

Run all five profiles through every registered scene with:

```sh
./bin/native-scene-scorecard-suite --enforce
```

This currently produces 70 scene and audio combinations. Passing proves the
declared response, motion coverage, global-pulse, pulse-duty, recovery, and
silence limits on the synthetic suite. It does not prove aesthetic quality,
genre-wide behavior, accessibility, or correct operation with every real
recording.
