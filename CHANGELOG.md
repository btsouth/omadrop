# Changelog

## Unreleased

- Add a persisted, paired-display color-safe palette on `C`. It preserves
  scene luminance structure, uses a restrained blue and gold range, and leaves
  album-cover color intact.

- Add Negative Space, a restrained eleventh native scene whose beat, kick,
  snare, hats, and section changes carve separate parts of one stable field.
- Add Ink Current, a selective fluid scene with local spectral shaping, beat
  travel, kick eddies, snare cuts, hat droplets, and structural branching.
- Add Glass Choir, a restrained harmonic scene with fixed refractive shards,
  moving internal caustics, localized percussion, and structural reassembly.
- Add Shadow Architecture, a stable 3D procession of lit concrete portals with
  separate floor, incision, lintel, beat, and section responses.
- Add Particle Weave, a fixed textile of braided beads with localized beat,
  kick, snare, hat, harmony, and section gestures.
- Add Living Mosaic, a stable field of stained cells whose seams, cuts, nuclei,
  and selected interiors carry separate musical roles.
- Add Lumen Fold, a calm installation of fixed translucent sheets with separate
  timing, floor, incision, suspension, and structural responses.
- Add Paper Horizon, a fixed cut-paper landscape with separate ridge, tear,
  star, lantern, moon, and section responses.
- Classify every scene by visual family and make automatic selection avoid
  recently shown families without changing manual navigation or motif recall.
- Add a deterministic 128-second director program test and prevent automatic
  family repeats or motif recalls before a scene has settled.
- Choose native transitions from scene compatibility, with separate flow,
  focal, depth, fracture, and negative-space paths.
- Add a hidden production-compositor gallery test for deterministic transition
  review without capturing the desktop.
- Cover every native scene with at least two incoming and two outgoing
  production-compositor transition paths, and tighten the reveal boundaries so
  detailed scenes do not become a muddy double exposure.
- Select compatible native transitions from the musical state at each boundary:
  carry motion into a rise, fracture on a harmonic turn, carve through a
  release, and preserve the focal structure in balanced passages.
- Add an atomic versioned preferences file with safe legacy ASCII migration and
  validated storage for display, motion, director, favorite, and hidden-scene
  settings.
- Add synchronized keyboard controls for intensity, brightness, ambient motion,
  reduced motion, and high contrast. Reduced motion limits feedback drift and
  transition travel without removing the music's localized timing cues.
- Show a brief synchronized status label after interactive changes. Manual
  scene skips say `AUTO: <scene>` so `N` and `P` cannot imply a hidden lock.
- Add Balanced, Kinetic, Restrained, and High Contrast director profiles that
  change scene choice without changing musical timing or response limits.
- Add synchronized scene favorites and hiding. Favorites modestly bias close
  automatic choices; hidden scenes leave automatic direction, motif recall,
  opening selection, and N/P navigation while preserving a two-scene minimum.
- Tighten the broad-pulse duty ceiling for flow scenes from 60 to 25 percent.
  Rework Spectral Ribbons so kicks, snares, and hats act on separate local
  windows instead of changing every line, with zero broad-pulse frames across
  all five deterministic replay profiles.
- Add a synchronized, persistent flash-limit control on `S`. It lowers fast
  event gain, caps requested brightness, compresses bright final output, and
  disables the optional contrast boost without removing local rhythm cues.
- Validate every scene and successor transition over a two-hour simulated
  1080p run: 432,000 frames, 1.33 ms p99, 6.76 ms maximum, no slow streak, and
  7.38 MiB resident-memory growth on the reference machine.
- Add `omadrop-check` as one pre-release gate for the native build, unit tests,
  shaders, transitions, synchronized launch, demo audio isolation, replay
  hashes, all 90 scene/profile combinations, and a 1080p stability soak.
- Define scene-pack format 1 with a checked-in example and a read-only
  `omadrop pack validate` command. Validation covers metadata, attribution,
  path and GPU-resource safety, bounded loops, API compatibility, compilation,
  silence, quiet-motion coverage, distinct transient roles, global pulsing,
  gesture recovery, and measured 720p frame time without installing the pack.
- Add `omadrop pack author` with automatic shader reload, a deterministic
  quiet-to-peak signal loop, visible kick/snare/hat/energy meters, seeking and
  pause controls, and simultaneous continuous and ASCII output.
- Add atomic install, list, and removal commands for validated community packs.
  The isolated store rejects links, special files, oversized content, and
  duplicate versions, and is never scanned by the official scene director.
- Record abnormal renderer exits atomically in a mode-600 local report with
  version, exit status, display count, and session type only. The report never
  captures audio, track metadata, artwork paths, process IDs, or output names.
- Keep the visualizer open when a new default audio output is temporarily
  unavailable. The active sink changes only after capture starts, silence stays
  still during retries, and a lost PipeWire recorder is detected and restarted.
- Add deterministic source packaging with clean-tree enforcement, safe archive
  member checks, the unified quick gate, an isolated install/pack/uninstall
  smoke test, exact VERSION-tag matching, and a SHA-256 checksum. Packaging
  never publishes the result.
- Replace the stale ten-scene website copy with a registry-aligned gallery of
  all 18 current scenes rendered from the native engine. The normal check now
  rejects missing, extra, renamed, or incorrectly sized gallery assets.
- Keep every paired control snapshot self-contained so it cannot erase an
  unread scene transition. Followers wait for the leader's first music frame,
  reject non-finite packets, and resynchronize autonomous flow every frame.
- Add a deterministic two-hour paired-display simulation with uneven frame
  timing and repeated dropped reads. Audio-state lag stays at or below 50 ms
  and scene-state disagreement clears within 34 ms without accumulated drift.
- Detect Linux suspend by comparing boot and active clocks. Resume clears
  stale analysis and buffered audio, refreshes MPRIS state, restarts capture on
  the current output, retains the visual composition, and limits flow advance.
- Add a hidden GPU compatibility probe that creates the required OpenGL 3.3
  context, compiles the complete native scene set, and renders a frame. The
  doctor now checks runtime commands, audio, linked libraries, GPU, and shaders.
- Add installed control and troubleshooting references with direct paths for
  audio, artwork, display, GPU, preference, and private-safe crash diagnosis.
- Lock two locally available demo tracks to verified CC BY 3.0 source records.
  Release recording now rejects an unknown or changed audio file and writes the
  complete source, license, change notice, and digest beside the accepted MP4.
- Add a real-song audit that rejects stalled or frantic automatic direction,
  invalid transition sequences, and motion that turns distinct musical roles
  into broad continuous pulsing.
- Tune Ink Current on both approved tracks so its beat and kick gestures remain
  clearly visible while changing less than two percent of the frame at once.
- Keep Depth Tunnel's deliberate continuous travel while moving percussion off
  its shared tunnel transform and into local shock, shutter, and glint regions.
  Add a second pulse-duty gate so moderate broad motion cannot become constant.
- Split Constellation Field into separate kick, snare, and hat node groups.
  Its real-song moderate-pulse duty falls from 35 to 7 percent while all three
  roles remain clearly above the response floor.
- Let Spectral Ribbons' low, middle, and high lines carry separate sustained
  frequency contours between attacks. Each contour stays in its own horizontal
  window, so melodic movement does not become another full-field pulse.
- Require every native scene to show spatially distinct sustained low, middle,
  and high-frequency material between attacks. Add local currents, petal
  details, void etchings, architectural courses, cell inlays, sheet light, and
  landscape accents to the seven scenes that did not meet that floor.
- Make scene galleries follow the registry so palette and grayscale review
  automatically includes new scenes.

## 0.3.0 - 2026-09-04

- Add an Omadrop-native HDR feedback renderer with ten original scenes as the
  default visual set: Depth Tunnel, Centrifuge, Wire Organism, Prism Garden,
  Orbital Loom, Tidal Grid, Pulse Cathedral, Constellation Field, Spectral
  Ribbons, and Bloom Engine.
- Derive a vivid three-color scene palette from album artwork, with a
  deliberate fallback palette when the artwork is neutral.
- Select new-section scenes by musical energy, percussion, harmony, spectral
  brightness, and stereo width while avoiding recently shown scenes.
- Coordinate paired-display startup behind a borderless staging frame so every
  cover or scene appears fullscreen at the same time.
- Wait for launch-time artwork before revealing either display, and never
  reverse from a visible scene into a late initial cover.
- Give ASCII covers a full-resolution image underlay, present clean covers in
  continuous mode, prepare new cache entries at 2048 px, use mipmapped
  filtering, and publish concurrent cover downloads atomically.
- Remember the selected ASCII or continuous display mode across launches.
- Fade the synchronized display pair in after placement and out on toggle or
  Escape instead of exposing hard window cuts.
- Synchronize ASCII mode, fullscreen mode, audio-delay controls, scene
  navigation, and keyboard-initiated shutdown from whichever display has focus.
- Add one-bar native scene transitions with manual next and previous controls
  and structure-aware automatic changes while preserving a center landmark.
- Keep next and previous as one-shot scene requests, then resume automatic
  direction after a minimum dwell with section, phrase, bar, and maximum-time
  fallbacks so a scene cannot remain stuck indefinitely.
- Add a renderer-facing music contract with 32-band spectrum, beat
  anticipation, downbeat, phrase, energy-direction, novelty, and section data.
- Add native harmonic/percussive texture estimates, spectral centroid, and
  stereo width, with distinct medium, focal-subject, and accent mappings.
- Feed album-cover edges and luminance structure into Depth Tunnel while
  keeping palette extraction available to every native scene.
- Add a deterministic native gesture audit for reaction strength and spatial
  separation, anticipation and downbeat visibility, event latency, frame
  continuity, and scene-landmark persistence.
- Audit moderate percussion and a song-like phrase, then shape normal-level
  kick, snare, and hat envelopes so quiet passages stay still while music reads
  clearly.
- Separate the native renderer's short visual hit envelopes from the analyzer's
  longer classification tails, and reduce constant flow so direct kick, snare,
  and hat deformation remains visually dominant.
- Give Spectral Ribbons distinct kick displacement, snare folding, and
  high-frequency ripples with faster feedback clearing at each transient.
- Add a silence-gated beat pulse to the native music contract and use it to
  reshape every scene's primary silhouette while reducing autonomous flow.
- Add a separate silence-gated onset pulse so ambiguous broadband attacks
  remain visible even when they are not classified as kick, snare, or hat.
- Add real-song motion auditing that compares beat, kick, snare, and hat
  attacks against genuinely quiet frames instead of relying only on isolated
  synthetic gestures.
- Randomize the synchronized native opening scene instead of always beginning
  with Depth Tunnel.
- Add `omadrop-demo`, a recording-ready five-scene showcase with a shortened
  cover intro, bar-aligned transitions, optional looping, and a clean close.
- Rank the showcase against real-song kick, snare, and hat response, promoting
  Wire Organism and Orbital Loom into the release sequence.
- Add `omadrop-demo-record`, which records the focused monitor from the cover
  intro through the final fade without capturing the surrounding desktop.
- Keep recorded demos aligned to their digital audio stream, remux captures to
  the video endpoint, and suppress notifications only for the capture.
- Stabilize MilkDrop `rand(...)` expressions during offline role audits so the
  projectM compatibility measurements remain reproducible.
- Add a structure-aware native scene director with development, drive, peak,
  release, deterministic scene-selection state, and topology recall for
  recurring musical sections.
- Synchronize the complete native `MusicFrame`, outgoing scene, and incoming
  scene from the leader to every paired display.
- Add a synchronized 720p GPU frame-time gate to the native render audit.
- Follow default output-device changes by restarting only the PipeWire capture
  child and loading the new sink's saved synchronization delay.
- Make the original native scene engine the default while retaining projectM
  through `OMADROP_ENGINE=projectm` for compatibility and A/B review.
- Add three Omadrop-authored presets: Contortion: Reactive Tunnel Edition,
  Halls Of Centrifuge: Reactive Orbit Edition, and Wire Dance: Reactive Wire
  Edition.
- Give kick, snare, hats, and sustained energy distinct internal geometry,
  color, border, and waveform responses.
- Preserve all three classic presets for credited live and offline A/Bs.
- Add a deterministic muted-viewer audit with per-role acceptance thresholds.
- Slow Reactive Orbit's autonomous rotation and accelerate it with detected
  percussion instead.
- Remove Cubetrace v2 from the curated rotation after live visual review.
- Remove Myriad Mosaics, Waterfowl in the Rain, Mandala Chasers, and shifter's
  Mandala after isolated-role and contact-sheet review.

## 0.2.0 - 2026-08-24

- Add optional song-structure timelines with MPRIS seek synchronization and
  repeated-section visual memory
- Add native phrase and arrangement-change tracking for restrained scene timing
- Recall visual families when similar musical entrances return
- Stabilize tempo across sparse, ambient, pop, and fast electronic material
- Expand the curated rotation from 6 to 16 ASCII-qualified presets
- Add six authored visual families with fresh variation on repeated sections
- Add deterministic preset audit metrics and contact-sheet generation
- Inhibit Omarchy's screensaver while Omadrop is visible
- Keep paired-monitor scene changes under one shared director
- Prevent short scene loops when a recalled visual family runs out of variants
- Give strong kick transients a clean, bounded geometry and glyph-weight accent
- Preserve album-cover composition and color with a restrained image underlay
- Remove a preset that could sustain a full-screen white field
- Remove a rainbow lattice preset whose autonomous motion obscured the music

## 0.1.1 - 2026-08-23

- Bundle the six curated presets and remove the classic projectM package dependency

## 0.1.0 - 2026-08-23

First public release.

- Native libprojectM renderer with optional Omadrop ASCII material
- PipeWire sink capture with kick, snare, hat, tempo, bar, and phrase analysis
- Six curated presets with authored music reactions
- Music-aware dual-renderer transitions
- MPRIS album covers with color-preserving ASCII rendering and ribbon dissolve
- Per-output audio synchronization settings
- Paired Hyprland monitor support and Omarchy shortcuts
- User-local Omarchy installer and uninstaller
- Deterministic audio, preset, adapter, queue, and motion regression tools
