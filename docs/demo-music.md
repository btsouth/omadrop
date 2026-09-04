# Demo music

Release recordings accept only an exact audio file listed in
`demo/music-rights.json`. `omadrop-demo-record` verifies its SHA-256 digest and
duration before capture, verifies the recorded opening against that source,
rejects competing audio streams, and writes the required credit next to the
finished MP4.

When an approved file is supplied, the recorder creates a temporary private
audio sink. The visualizer and recorder listen only to that sink, and the
approved player is routed into it. Browser, notification, and other desktop
audio remain on the user's normal output and cannot enter the visual analysis
or recording. The private sink is removed on success, failure, or interruption.

Recordings also disable desktop media-session discovery and begin with
Omadrop's checked-in showcase card. This prevents an unrelated browser or
music player from supplying the title, playback state, or cover art. Set
`OMADROP_DEMO_COVER_FILE` only when a different, rights-cleared local image has
been deliberately approved for the recording.

## Approved tracks

### beat me

- Creator: smilingcynic (Christopher Hawes)
- Source: [ccMixter](https://ccmixter.org/files/smilingcynic/41102)
- License: [Creative Commons Attribution 3.0 Unported](https://creativecommons.org/licenses/by/3.0/)
- Intended review role: fast electronic rhythm and dense peak response

### To Free Me (Instrumental)

- Creator: Ivan Chew
- Source: [ccMixter](https://ccmixter.org/files/ramblinglibrarian/47788)
- License: [Creative Commons Attribution 3.0 Unported](https://creativecommons.org/licenses/by/3.0/)
- Intended review role: slower live drums, guitars, and long-form dynamics

The audio files are not distributed with Omadrop. License and source pages were
verified on 2026-09-04. CC BY 3.0 permits sharing and adaptation, including
commercial use, when appropriate credit and the license link are provided.
