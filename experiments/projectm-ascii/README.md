# Native projectM to ASCII renderer

This directory contains Omadrop's active rendering architecture:

1. libprojectM renders a real `.milk` preset into an OpenGL framebuffer.
2. The frame is sampled as 2x4 braille subcells.
3. Each dot pools its full source area so thin MilkDrop geometry survives.
4. Native and ASCII-style frames are exported for direct comparison.

Build:

```sh
./experiments/projectm-ascii/build.sh
```

Run the native music-contract and render audits with:

```sh
./experiments/projectm-ascii/music-frame-test
./experiments/projectm-ascii/signal-monitor-test
./experiments/projectm-ascii/status-overlay-test
./experiments/projectm-ascii/live-compositor-test
./experiments/projectm-ascii/cover-presentation-test
./experiments/projectm-ascii/mpris-state-test
./experiments/projectm-ascii/mpris-poller-test
./experiments/projectm-ascii/audio-output-session-test \
  ./experiments/projectm-ascii/fixtures/fake-pw-record
./experiments/projectm-ascii/paired-transport-test
./experiments/projectm-ascii/paired-sync-test
./experiments/projectm-ascii/session-lifecycle-test
./experiments/projectm-ascii/native-scene-state-test
./experiments/projectm-ascii/native-scene-list
./experiments/projectm-ascii/native-renderer-test ./shaders/native
./experiments/projectm-ascii/native-transition-test ./shaders/native \
  ./experiments/projectm-ascii/live.cpp
./experiments/projectm-ascii/native-renderer-soak ./shaders/native 10
./bin/native-director-program-test
./bin/omadrop-launcher-test
./bin/demo-audio-audit-test
```

The launcher test uses two deterministic fake displays and renderers. It
verifies that both windows become ready before either is revealed, each PID is
routed to the intended monitor and made fullscreen, the first exit terminates
its sibling, and all runtime synchronization files are removed. It also removes
one display at runtime and verifies that the old pair closes before a correctly
routed one-display session is revealed.

The paired-sync test simulates two hours at independent, uneven display-frame
rates with repeated two-read gaps. It verifies monotonic validated music
packets, bounded beat/bar/phrase and autonomous-flow lag, self-contained scene
and control snapshots, topology-session restart, and no accumulated drift.

The session-lifecycle test distinguishes Linux suspend time from a slow active
frame. On resume the live renderer retains its composition, clears buffered and
transient audio state, refreshes track state, and restarts PipeWire capture.
The renderer audit also verifies that a long pause cannot advance scene flow by
more than the normal 100 ms frame clamp.

`native-scene-list` validates registry order, identity, and uniqueness before
printing the canonical slug and display name for every scene. Gallery and
scorecard tooling consume this output, so adding a registry entry automatically
adds the scene to every visual and music-response review.

The audio-output session test uses a fake capture process. It verifies the
exact sink passed to `pw-record`, orderly capture replacement, state reset, a
per-output delay change, and a clean failure when the capture executable cannot
start.

The demo-audio audit test accepts a capture with the approved opening at a
known delay and rejects a different source. This protects the release recorder
from silently accepting desktop audio that does not belong in the demo.

The live-compositor test owns the final display pass in a hidden OpenGL
context. It verifies texture presentation and the launch and shutdown
visibility endpoints independently from the audio loop. Cover blending,
continuous and ASCII materials, backend transitions, and display fades now
cross one typed frame interface.

The native transition test extracts the production compositor shader and
renders the five authored grammars without opening a visible window. Its
coverage graph gives every registered scene at least two incoming and two
outgoing paths, checks both halves of every transition, and can write review
frames when an output directory is supplied. Native reveal boundaries stay
narrow enough to preserve both compositions instead of producing a long
double exposure. Four additional paths verify that calm releases, harmonic
turns, rising sections, and balanced passages select distinct compatible
grammars. The director captures that choice once at the musical boundary, so
individual hits cannot change the transition underneath the viewer.

The MPRIS poller test exercises the asynchronous helper lifecycle, valid and
invalid state parsing, the no-player result, duplicate-start protection, and
cleanup. The render loop consumes completed track observations without owning
fork, pipe, buffering, or child-process state.

The cover-presentation test locks the complete artwork lifecycle: initial
hold, smooth dissolve, completion, synchronized restart after the display
gate, explicit removal, and replacement. A track change clears the previous
cover before attempting to load the next one, so missing artwork cannot leave
the old album image on screen.

The renderer soak runs at 1920x1080 by default, cycles through every scene and
the transition to its successor, and synchronizes each frame for real GPU
timing. It fails on OpenGL errors, a 99th-percentile frame time above 18.5 ms, a
continuous second below 55 FPS, or more than 64 MiB of resident-memory growth
after warmup. The final argument is simulated minutes from 1 to 240.

The native director program concatenates all five locked audio profiles into a
128-second deterministic performance and runs it through the real analyzer,
structure detector, director, and renderer. It locks the automatic scene
sequence, requires at least eight seconds between completed scene changes, and
prevents adjacent visual-family repetition. It also locks the transition style
chosen at each boundary from the measured musical state.

Pass an optional output directory to `native-renderer-test` to write one
deterministic continuous frame and one production-equivalent ASCII frame per
native scene for visual review. The ASCII reference exercises the native,
no-cover, no-transient branch of the live display material at 1280x720.

Replay a full 44.1 kHz stereo f32 song through the complete native analyzer,
director, and renderer with `native-song-replay SHADERS RAW OUTPUT_DIRECTORY`.
It writes a two-second visual contact sequence plus a per-frame music timeline.
`OMADROP_REPLAY_CAPTURE_HOPS` changes the capture interval from its 120-hop
default, and `OMADROP_REPLAY_MAX_SECONDS` limits a motion-review excerpt.
`OMADROP_REPLAY_WIDTH` and `OMADROP_REPLAY_HEIGHT` select a review resolution
between 320x180 and 1920x1080.
`OMADROP_REPLAY_INTENSITY` and `OMADROP_REPLAY_MOTION` exercise the saved live
control ranges in deterministic review. Set
`OMADROP_REPLAY_REDUCED_MOTION=1` to apply the same ambient-motion cap used by
the live renderer.

Set `OMADROP_REPLAY_SIGNAL_MONITOR=1` to add the developer signal monitor to
replay frames and encoded review videos. Its eight lanes, from top to bottom,
show beat phase, bar phase, clock confidence, kick, snare, hat, rhythmic
density, and section novelty. Phase lanes use moving cursors; the remaining
lanes use level bars. The monitor exists only in deterministic replay and can
never appear during normal playback. Replay timelines also include BPM and
clock confidence as numeric columns.

Render every registered scene across one song and assemble per-scene contact
sheets plus a five-point comparison matrix with:

```sh
./bin/native-song-gallery song.f32 /tmp/omadrop-song-review 60
```

The output directory must be empty. The optional final argument limits the
review to that many seconds.

Encode the exact 60 FPS replay with its matching audio for continuous-motion
review with:

```sh
./bin/native-song-video song.f32 /tmp/depth-review.mp4 depth-tunnel 30
```

The encoder streams frames through a FIFO, so it does not leave a directory of
raw frames behind. Set `OMADROP_VIDEO_WIDTH` and `OMADROP_VIDEO_HEIGHT` to
change the default 960x540 review resolution.

Generate the deterministic structured fixture and measure every native scene
with the same input:

```sh
./experiments/projectm-ascii/scorecard-fixture /tmp/omadrop-scorecard.f32
./bin/native-scene-scorecard /tmp/omadrop-scorecard.f32
```

With no arguments, `native-scene-scorecard` generates the fixture itself and
writes HTML-friendly Markdown plus raw TSV data to
`cache/native-scene-scorecard`. Add `--enforce` to fail when any scene misses
the 1.75x kick, snare, or hat response floor or exceeds the 0.003 absolute
silence-drift ceiling. It also enforces per-scene non-transient motion coverage
budgets for sparse, selective, and flow motion grammars, so broad pulsing does
not become the default reaction. A global-pulse gate also rejects reactions
that move a large part of the frame in the same brightness direction. A pulse
duty gate prevents sparse and selective scenes from spending more than 12
percent of their frames above a 20 percent global-pulse score. Flow scenes have
a separate 25 percent ceiling, enough for deliberate broad travel without
turning steady audio into continuous whole-frame pulsing. The scorecard is a
regression gate. Still frames, motion, palettes, continuous rendering, and
ASCII rendering still require human visual review.

The locked replay suite adds sparse acoustic-like, dense compressed,
sustained vocal-like, and syncopated fast-section probes to the structured
electronic baseline. The waveforms are synthesized entirely by repository
code. Verify their reference hashes and run all 90 current scene combinations
with:

```sh
./bin/native-replay-fixtures --verify
./bin/native-scene-scorecard-suite --enforce
```

See [the fixture specification](../../docs/replay-fixtures.md) for scope and
limitations.

Render the production tone map across six difficult album colors, plus a
grayscale review sheet, for both continuous and ASCII materials with:

```sh
./bin/native-scene-gallery /tmp/omadrop-native-gallery
```

Run:

```sh
preset="presets/curated/Omadrop + Aderrasi - Contortion (Reactive Tunnel Edition).milk"
./experiments/projectm-ascii/projectm-ascii "$preset" /tmp/native.ppm /tmp/ascii.ppm
```

`projectm-ascii` remains the deterministic offline comparison tool.
`projectm-ascii-live` is the live Omadrop renderer.

Run the muted-viewer reaction audit with:

```sh
./bin/reactivity-audit
```

It compares the preserved classic Contortion, Halls Of Centrifuge, and Wire
Dance presets with Omadrop's authored editions under isolated kick, snare, and
hat fixtures.
The audit fails if any role does not create a strong coarse-motion gesture or
does not improve materially over the corresponding classic preset.

Replay a raw 44.1 kHz stereo f32 sink capture through the production analyzer
with `audio-feature-replay CAPTURE.raw`. It reports percussion, tempo, clock
confidence, bar novelty, phrases, and structural boundaries for repeatable
real-song tuning.

## Live GPU proof

`projectm-ascii-live` captures the default PipeWire sink, feeds PCM directly to
libprojectM, copies projectM's completed frame into a GPU texture, and applies
the ASCII dot composite in an OpenGL shader:

```sh
preset='presets/curated/Aderrasi + Geiss - Airhandler (Kali Mix) - Painterly Kaleidoscope 2.milk'
./experiments/projectm-ascii/projectm-ascii-live "$preset"
```

Run the first curated rotating set with:

```sh
./experiments/projectm-ascii/run-curated.sh
```

Once the native beat clock is reliable, scenes breathe for at least six bars.
Sustained arrangement changes can then start a one to two bar dual-renderer
morph. If no strong change arrives, a phrase boundary after twelve bars advances
the scene. Low-confidence material keeps the bass-onset and deadline fallback.
Strong boundaries are spaced by at least twenty-four seconds. Similar spectral
entrances recall the same visual family while choosing a fresh compatible
preset, creating theme and variation instead of exact replay. Each preset
continues reacting to individual hits throughout. Press `n` to skip immediately
and Escape to quit. `[` and `]` adjust audio/video
alignment by 10 ms while listening. `OMADROP_SYNC_MS` sets the initial delay;
Bluetooth outputs default to 180 ms and wired outputs to 35 ms. Manual changes
are saved per output in `$XDG_CONFIG_HOME/omadrop/sync-by-sink/`, so switching
between Bluetooth and wired devices preserves separate calibration values.

Press `p` to return to the previous preset while auditioning the curated set.
Automatic changes choose within the track's slowly measured calm or driving
energy class, with a small variation among the strongest compatible candidates.
The current preset and seven most recent presets are excluded. If a recalled
family has no fresh variant, selection widens to its related visual group before
reusing a recent scene.

Press `a` to switch between Omadrop ASCII and the original MilkDrop rendering.
Press `i` to cycle local response intensity, `b` for brightness, `m` for
ambient motion, `r` for reduced motion, and `h` for high contrast. Reduced
motion caps feedback drift and transition travel while leaving the local
percussion, beat, and section cues readable. Press F11 to toggle fullscreen.
Press `s` for flash limit. It lowers fast event gain, caps requested brightness,
compresses bright final output, and prevents the high-contrast boost while
keeping localized timing cues active.
Press `d` to cycle Balanced, Kinetic, Restrained, and High Contrast direction.
Profiles change candidate scoring while preserving the same musical boundaries
and per-scene response limits.
Press `f` to favorite or unfavorite the current scene. Favorites receive a
small preference when multiple candidates fit the music, but they do not force
a poor musical match. Press `x` to hide the current scene and continue to the
next visible scene. Hidden scenes are omitted from automatic selection, motif
recall, and `n`/`p` navigation. Press `Shift+x` to restore them. The director
refuses a hidden set that would leave fewer than two scenes available.
These choices are stored in the versioned
`$XDG_CONFIG_HOME/omadrop/preferences.conf` file. Version 1 preferences load
with flash limit off and are upgraded on the next write. Legacy `ascii-enabled`
settings migrate automatically, and settings writes replace the complete file
atomically so paired displays never observe a partial update.

Every interactive change gets a brief bottom-center status label. Manual scene
navigation reads `AUTO: <scene>` to make its one-shot behavior explicit. The
overlay uses a small built-in bitmap font and its own final OpenGL pass, so it
does not add a font dependency or alter the visual feedback buffers. Its raster,
render, placement, and expiry paths run in a hidden-context regression test.

Press Escape to exit. The process prints `audio: PipeWire` when nonzero sink
samples arrive. `OMADROP_SYNTHETIC_AUDIO=1` forces a deterministic repeating
kick, snare, and hat fixture. `OMADROP_DISABLE_ART=1` skips the cover sequence,
`OMADROP_COVER_PATH` forces a local cover for visual comparison, and
`OMADROP_AUTO_NEXT_MS` requests one preset transition after the given number of
milliseconds. `bin/motion-capture` uses these controls to inspect the native SDL
renderer without manual input. `OMADROP_REACTION_SCALE=0` disables the authored
post-process movement for deterministic native-versus-authored A/Bs.
`OMADROP_CLASSIC_CONTORTION=1` swaps the preserved classic Contortion preset
back into the rotation for a live A/B against the Reactive Tunnel edition.
Add `OMADROP_CONTORTION_ONLY=1` to hold either edition for a focused listening
test instead of waiting for it to appear in the rotation.
`OMADROP_CLASSIC_HALLS=1` and `OMADROP_HALLS_ONLY=1` provide the same A/B for
Halls Of Centrifuge and its Reactive Orbit edition.
`OMADROP_CLASSIC_WIRE=1` and `OMADROP_WIRE_ONLY=1` provide the same A/B for
Wire Dance and its Reactive Wire edition.

The registered native scenes are the default renderer. They use the
production audio, artwork, palette, ASCII, and windowing paths through
Omadrop's HDR feedback backend. Press `N` or `P` to transition between scenes.
Their versioned definitions live in `native_scene_registry.h`. A definition
owns the scene's identity, shader, materials, declared musical roles, selection
traits, visual family, transition anchor, and performance limit. The renderer
and director both consume this registry, so a scene does not require a separate
hardcoded shader or selection entry. Automatic selection penalizes the visual
families used by recent scenes while preserving exact manual navigation and
motif recall.

The native transition test extracts the production compositor shader, renders
all five authored transition grammars in a hidden OpenGL context, and verifies
that both halves of every path develop visibly. Pass an output directory to
write six review frames per transition.
Set `OMADROP_ENGINE=projectm` to run the preserved preset renderer for
compatibility or A/B review. `OMADROP_NATIVE_SCENE` accepts `depth-tunnel`,
`centrifuge`, `wire-organism`, `prism-garden`, `orbital-loom`, `tidal-grid`,
`pulse-cathedral`, `constellation-field`, `spectral-ribbons`, `bloom-engine`,
`negative-space`, `ink-current`, `glass-choir`, `shadow-architecture`,
`particle-weave`, `living-mosaic`, `lumen-fold`, or `paper-horizon`.
Set `OMADROP_ASCII=0` to inspect the continuous native field without the final
ASCII material.

## Structure timelines

Timelines are an optional authoring and test override. Normal Spotify, browser,
and local playback uses the native live detector and needs no timeline or AI
runtime.

`OMADROP_TIMELINE_PATH` loads an optional full-song JSON timeline. While it is
active, section boundaries replace randomized dwell scheduling. Each section
identity receives a stable preset, so later occurrences of the same identity
return to the same visual family. MPRIS position keeps the timeline aligned
through pause and seek, and a seek across sections restores the target preset
immediately.

```sh
OMADROP_TIMELINE_PATH=./experiments/projectm-ascii/fixtures/manual-song.json \
  ./experiments/projectm-ascii/run-curated.sh
```

The optional `track.identity` must exactly match the player's MPRIS
`xesam:url`. Omit it only for a deliberately unbound test fixture. For a
frozen visual check, `OMADROP_TIMELINE_POSITION` overrides the player position
with a non-negative number of seconds. Invalid timelines fail at startup. If
no timeline is configured, or its identity does not match, the existing live
audio director remains active.

The minimum schema is:

```json
{
  "duration": 100.0,
  "track": {"identity": "file:///music/example.mp3"},
  "sections": [
    {"start": 0.0, "end": 20.0, "identity": "A", "label": "intro"},
    {"start": 20.0, "end": 40.0, "identity": "B", "label": "verse"}
  ]
}
```

Album art comes from MPRIS, holds for five seconds, then dissolves over five
seconds in coordinated ribbons. Each braille cell averages the cover's RGB and
luminance, applies a restrained cell-local tone curve, and caps dot activation
below a solid field. A dim original-color underlay preserves faces, typography,
and fine texture while the glyphs remain dominant. The underlay fades early in
the dissolve, and cover reactions stay restrained so hard hits do not distort
the artwork.

## Current findings

- The system libprojectM 4 C API embeds cleanly in an SDL OpenGL context.
- The installed preset corpus contains more than 4,000 `.milk` files.
- Coherent MilkDrop structures survive a braille-style final pass.
- Point sampling loses 1px waves and outlines. Per-dot max pooling preserves
  them and should inform the production GPU composite shader.
- The old hardcoded procedural scenes are no longer the product path.
- A live, GPU-only projectM-to-ASCII handoff works in an SDL OpenGL window.
- Per-preset family and reaction profiles are required. A universal overlay
  cannot make every MilkDrop composition feel musically intentional.
- Hit envelopes preserve transient strength, so harder detected hits produce
  stronger preset gestures instead of the same binary pulse.
- Album covers and preset frames need different color sampling. Brightest-pixel
  pooling preserves preset hairlines but can wash out cover palettes.

## Preset qualification

The review harness now uses the same 12x24 braille cell geometry, level
quantization, and authored exposure values as the live renderer. Build it, then
create a deterministic metrics table and labeled contact sheet:

```bash
./bin/preset-audit cache/preset-audit presets/curated/*.milk
```

This tooling is for maintainers. It adds no runtime service, account, model, or
AI dependency to Omadrop.
