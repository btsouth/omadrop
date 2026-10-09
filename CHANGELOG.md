# Changelog

## 0.7.0

- New world: Kanagawa, a sunset Hokusai print with Fuji, a great wave and two
  rowing crews. Loud music builds each set: the wave grows, feints on strong
  hits and breaks, and the crews race it off the right edge. Sets differ, from
  towering waves to ones that fizzle out or break early, and the sea stays calm
  for a while after each landing. Kanagawa plays in Omarchy mode when your
  Omarchy theme is Kanagawa.
- Omarchy mode now picks the world that matches your Omarchy theme and falls
  back to Osaka Jade.
- Build a living world for your theme. `omadrop world new` starts one from a
  template, `omadrop world preview` reloads it as you save, and
  `omadrop world check` reports whether it loads, reacts to music, fits the
  frame budget and avoids harsh flashing. Worlds are a `scene.json` and an
  Inkscape `art.svg` built from a shared library of pieces. See
  [Build a living world](docs/worlds.md).
- Osaka Jade now loads from its own world folder, with no visible change.

## 0.6.3

- Fix MilkDrop mode with "All screens" selected on more than one display. The
  second window opened as a small window on the focused display instead of
  full screen on its own display.

## 0.6.2

- Osaka Jade renders more than twice as fast on integrated graphics without
  changing how it looks. On an Intel UHD 770 at 1080p a frame now takes about
  30 ms instead of 69 ms.
- Slower GPUs keep full resolution at a steady 30 fps instead of dropping to a
  lower resolution. Resolution only steps down when 30 fps at full resolution
  is out of reach. Fast GPUs stay at 60 fps.

## 0.6.1

- Fix MilkDrop mode not starting with "This screen" selected. Omadrop now
  matches Hyprland's monitor names to the names SDL reports, such as
  `DP-2 27"`, which also fixes MilkDrop on all displays where the names differ.

## 0.6.0

- Omarchy mode is now Osaka Jade, a living street that reacts to your music.
  It plays on every Omarchy theme, and Esc returns to the controls. MilkDrop
  and its ASCII option remain the default.
- Every launch gets its own timeline. Trains, cyclists, the noodle cart, tea,
  toasts, gusts and the shooting star come back at varied times and speeds,
  and the street's small gestures and windows no longer repeat on fixed loops.
- Fireworks follow the music. Loud songs get a full show with follow-up
  shells; quiet songs get a single small shell. Each waits 45 to 90 seconds
  before the next.
- Osaka lowers its internal resolution on slower GPUs, such as integrated
  Intel graphics, and restores it when there is headroom. Set
  `OMADROP_OSAKA_SCALE` to a value from 0.5 to 1 to fix the scale.
- Osaka keeps reacting at low playback volume. Quiet desktop audio is
  lifted before analysis, so bass hits, moon rings and the full firework show
  no longer depend on your playback volume.
- The mouse pointer hides as soon as the visuals start, without needing a
  nudge, and reappears briefly when you move it.
- "This screen" plays on the monitor you are using rather than the first one
  Hyprland lists.
- The Omarchy tab in the controls shows an Osaka Jade preview.
- Remove the ttfx terminal effects, the effect browser and their terminal
  dependencies. Upgrades clean up the old screensaver commands and launcher.

## 0.5.0

- One app with controls first and a compact thumbnail browser.
- Omarchy mode with all 37 music-driven ttfx terminal effects.
- Hide scenes and effects, favorite effects, and pick a scene to start playback.
- ASCII previews for the 21 curated MilkDrop scenes.
- Shorter MilkDrop scene holds of 14 to 20 seconds and scene captions.
- One Arch package containing the controls and both backends.
- Esc returns to controls from both playback modes.
- Next and previous effect in Omarchy mode with N, P and the arrow keys.
- No album art cover; Play reaches fullscreen visuals in about a second.
- Controls follow the full Omarchy theme and fade when it changes.

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
