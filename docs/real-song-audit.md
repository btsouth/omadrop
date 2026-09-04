# Real-song release audit

Omadrop is checked against complete songs as well as synthetic signal fixtures.
The source audio stays local and is not distributed with the project.

## Approved music

| Track | Source | License | SHA-256 |
| --- | --- | --- | --- |
| beat me, smilingcynic (Christopher Hawes) | [ccMixter](https://ccmixter.org/files/smilingcynic/41102) | [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/) | `37e50379d9af0b43d586e429a122776b23f09f9412c5052e5e1237c3b127f5fa` |
| To Free Me (Instrumental), Ivan Chew | [ccMixter](https://ccmixter.org/files/ramblinglibrarian/47788) | [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/) | `32260c101e55466e051c65fd9ecd98cfa3b35d83c428b23bf9afab2e25991e78` |

The exact file hash, duration, source, creator, and license are locked in
[`demo/music-rights.json`](../demo/music-rights.json). `demo-rights-audit`
rejects any different file.

## 2026-09-04 result

Both complete tracks pass all 18 native scene gates. Each scene has separate
kick, snare, and hat movement, near-still silence, bounded quiet motion, fast
gesture recovery, and no broad continuous pulse.

| Result | beat me | To Free Me |
| --- | ---: | ---: |
| Scenes passing | 18/18 | 18/18 |
| Highest mean global pulse | 11.98% | 9.96% |
| Highest broad-pulse duty | 15.01% | 9.21% |
| Weakest kick response | 2.09x | 2.75x |
| Weakest snare response | 1.81x | 2.34x |
| Weakest hat response | 2.06x | 2.37x |
| Highest silence drift | 0.00117 | 0.00117 |

Ink Current initially failed the first track because its beat and kick were too
subtle. Its tuned gestures now exceed 2.30x quiet motion while each transient
changes less than two percent of the image. The correction is localized and
does not add whole-frame scale, flash, bounce, zoom, or shake.

The automatic director also passes both songs:

| Result | beat me | To Free Me |
| --- | ---: | ---: |
| Duration | 201.0 s | 170.2 s |
| Transitions | 14 | 9 |
| Distinct scenes | 9 | 7 |
| Shortest gap | 6.9 s | 8.8 s |
| Longest gap | 17.1 s | 23.9 s |
| Transition blend | 2.0 s | 2.2 s |

This rejects two opposite failures: a scene that remains indefinitely and a
director that changes constantly. Every completed change also has a valid
authored transition style and target.

## Reproduce locally

With exact approved audio files available locally:

```bash
./bin/native-approved-music-audit cache/approved-audit \
  /path/to/beat-me.mp3 /path/to/to-free-me.mp3
```

The command verifies rights first, converts audio to the renderer's replay
format in a temporary directory, runs the complete scene and director audits,
and writes evidence without copying the music.
