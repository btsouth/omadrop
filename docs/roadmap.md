# Omadrop roadmap after v0.3

## Product direction

Omadrop should turn a song into an authored visual performance. It must respond
at three time scales:

1. **Attacks:** kicks, snares, hats, and other transients change the image
   immediately and differently.
2. **Groove:** beats and bars establish repeated motion that makes the rhythm
   clear without reducing the scene to a spectrum display.
3. **Arrangement:** phrases, sections, breakdowns, returns, and peaks change
   composition, density, color, and scene choice.

The renderer should be nearly still when the audio is silent. Music must change
the scene's own geometry and behavior. Generic flashes, rings, zooms, and camera
shake are not substitutes for scene-specific response.

## Rules that do not change

- Ship fewer scenes when the alternatives are weaker.
- A new scene needs a distinct composition and motion system, not a new palette
  on an existing shader.
- Every scene needs a stable subject or spatial structure that can be followed
  over time.
- Color supports form. A scene must still read in grayscale.
- ASCII is a material applied to the same high-quality continuous image.
- Automatic direction respects the song. It does not change scenes to meet a
  timer when a musical boundary is available.
- Silence produces almost no activity.
- Launch, display synchronization, controls, cover art, and shutdown remain as
  polished as the visuals.
- Audio analysis stays local and deterministic.
- Automated metrics catch regressions. Human review decides whether a visual is
  beautiful enough to ship.

## The quality gate

Current scene-by-scene findings are tracked in the
[native scene quality matrix](scene-quality.md).

Every scene, transition, and release must pass the same review.

### Music response

- Declare at least three primary musical inputs and the visible role of each.
- Produce a measurable response to each declared input within 100 ms of its
  presentation time.
- Keep each role spatially or behaviorally distinct from the others.
- Do not scale, flash, bounce, zoom, or shake the whole composition for every
  onset. Broad motion is allowed only when it is a deliberate role in that
  scene, not the default response to any sound.
- Measure the fraction of the frame changed by each transient. Similar broad
  coverage for kick, snare, and hat requires visual review even when response
  strength passes.
- Require transient gestures to decay or resolve on their intended musical
  time scale instead of accumulating into constant pulsing or jitter.
- Keep quiet-frame motion below 15 percent of typical active motion.
- Avoid continuous high response caused by long envelopes or false onsets.
- Preserve readable beat motion across sparse acoustic, dense electronic, and
  heavily compressed mixes.

The initial quantitative floor is 1.75 times quiet-frame motion for each
declared transient response. Existing baselines will determine stricter
per-scene thresholds. A high score does not compensate for ugly motion.

### Visual composition

- The still frame has a deliberate focal structure, balance, and negative
  space.
- Motion develops the composition instead of erasing it every frame.
- Brightness stays controlled during feedback accumulation.
- Album-derived palettes remain legible for neutral, dark, and oversaturated
  covers.
- Continuous and ASCII materials both preserve the identity of the scene.
- The scene remains strong during quiet, developing, driving, peak, and release
  passages.

### Variety

Review each scene on these axes:

| Axis | Examples |
| --- | --- |
| Composition | radial, horizontal, perspective, figure and ground, full field |
| Matter | line, surface, particle, fluid, volume, glyph |
| Motion | orbit, flow, growth, fracture, folding, wave, camera travel |
| Density | sparse, layered, dense |
| Depth | flat graphic, shallow field, deep space |
| Energy character | restrained, elastic, sharp, heavy, explosive |

A candidate does not ship when it matches an existing scene on most axes.
Contrast across the rotation matters as much as the quality of one scene.

### Operational quality

- No desktop, title bar, intermediate window, or setup frame is visible at
  launch or shutdown.
- Multi-monitor windows show the same musical state and control response.
- Manual scene changes do not disable automatic direction.
- Saved preferences survive updates and output-device changes.
- The 99th-percentile frame time stays below 18.5 ms at 1080p on the reference
  machine, with no sustained drop below 55 FPS at native display resolution.
- A deterministic recording contains only its approved audio source.

## Phase 0: v0.3.1 quality floor

Do not add scenes yet. Establish a reliable baseline for the ten that exist.

### Work

- Build a locked, rights-cleared replay set covering sparse percussion, dense
  rock, electronic music, acoustic music, vocals, quiet intros, breakdowns, and
  fast section changes.
- Add a scene scorecard that records role response, motion coverage, recovery,
  false activity, frame pacing, brightness, continuity, and transition
  behavior.
- Raise Spectral Ribbons and Constellation Field to the response level of the
  strongest current scenes without destroying their form.
- Review all ten scenes in grayscale, continuous color, ASCII, and at least six
  album-derived palettes.
- Add regression cases for launch while a track is already playing, pause,
  seek, track change, output change, display hotplug, and shutdown.
- Keep the new recording audio guard and add an automated opening-audio check.
- Fix installation and GPU compatibility issues reported after the v0.3 post.

### Exit criteria

- All ten scenes pass their declared response thresholds.
- No current scene fails the still-frame composition review.
- The launch and ending remain clean in every test configuration.
- There are no known release-blocking install, display, audio, or persistence
  defects.

## Phase 1: v0.4 music perception

Improve the information available to every scene before increasing the scene
count.

### Work

- Normalize transient and band response across quiet, loud, compressed, and
  bass-heavy masters.
- Improve downbeat, tempo, and half-time or double-time stability.
- Add rhythmic density, syncopation, spectral contrast, and tonal movement to
  `MusicFrame`.
- Add chroma and harmonic-change signals for color and structural movement.
- Separate short attacks from sustained percussion more reliably.
- Improve online phrase and section detection without requiring a full-song
  preprocessing pass.
- Add an optional developer signal monitor that overlays timing and confidence
  during replay, never during normal playback.
- Calibrate end-to-end presentation delay per output with a repeatable tool.

### Exit criteria

- The beat clock remains stable through intros, breakdowns, and tempo
  ambiguity.
- Section events occur on useful musical boundaries across the replay set.
- Existing scenes improve or remain unchanged under deterministic A/B review.
- No visual backend reads analyzer internals outside `MusicFrame`.

## Phase 2: v0.5 visual platform

Make the renderer easy to extend without turning it into a collection of
special cases.

### Work

- Split the current application loop into audio, track state, display session,
  cover presentation, director, and render modules.
- Replace hardcoded scene switches with a versioned scene registry containing
  identity, materials, musical roles, selection traits, transition anchors,
  and performance limits.
- Give each scene persistent CPU-side state where its composition needs memory
  beyond the shared feedback buffers.
- Add shader hot reload and deterministic replay controls for development.
- Build shared primitives for flow fields, particles, curves, surfaces,
  refraction, signed-distance geometry, and feedback transport.
- Formalize a three-role palette system for background, body, and accent while
  keeping scene-specific color behavior.
- Define transition inputs that allow scenes to expose a focal point, axis,
  depth field, or motion vector to the incoming scene.
- Add GPU timing per render pass and automatic quality scaling that preserves
  the composition.

### Exit criteria

- The current ten scenes render identically within the golden-image tolerance.
- A new scene can be added through the registry without editing launch,
  director, input, or multi-monitor code.
- Transitions can preserve a declared landmark or motion direction.
- Performance diagnostics identify the cost of each active pass.

## Phase 3: v0.6 visual expansion

Prototype broadly, then ship only the strongest work. The target is 18 to 20
excellent native scenes, not the largest possible count.

### Candidate families

1. **Ink Current:** fluid calligraphy with bass-driven flow, kick vortices,
   snare cuts, and hat droplets.
2. **Glass Choir:** refractive vertical forms whose harmonic content changes
   shape while percussion creates controlled fractures.
3. **Shadow Architecture:** deep, sparse structures lit by onsets, with bars
   changing perspective and sections rebuilding the space.
4. **Particle Weave:** independent particle threads for rhythmic roles that
   braid together across a phrase.
5. **Kinetic Relief:** a moving topographic surface where groove controls
   traversal and arrangement changes reshape the terrain.
6. **Cellular Bloom:** organic growth and division tied to sustained energy,
   with transients changing growth direction rather than adding flashes.
7. **Glyph Weather:** a native glyph field whose density, flow, and grouping
   respond to musical structure. It must work as a full composition, not an
   overlay.
8. **Negative Space:** a restrained scene where the music carves darkness out
   of a luminous field. This provides contrast with dense feedback scenes.
9. **Vector Storm:** sharp directional forms that make syncopation and stereo
   movement visible.
10. **Chroma Tides:** broad layered color fields driven by tonal movement and
    harmonic change, with percussion affecting boundaries rather than the whole
    frame.

Build at least two rough candidates in each broad visual grammar. Promote at
most one when the alternatives are too similar. A candidate is cut if it does
not look strong in a still, does not move distinctly, or duplicates an existing
scene.

### Exit criteria

- The rotation contains at least two strong sparse scenes, two deep scenes, two
  surface or fluid scenes, two particle or line scenes, and two high-energy
  scenes.
- Every addition passes the full quality gate and has a clear reason to exist.
- A blind contact sheet and motion review can distinguish every shipped scene.
- The full rotation stays within the frame-time budget.

## Phase 4: v0.7 director and transitions

Turn a sequence of scenes into a coherent performance of the whole song.

### Work

- Score candidates by instrumentation, energy direction, rhythmic density,
  stereo width, tonal motion, current palette, visual density, and recent use.
- Plan contrast across multiple scene choices instead of selecting only the
  next scene.
- Recall a scene family when a chorus or motif returns, while allowing its
  details and intensity to develop.
- Distinguish intros, verses, choruses, bridges, breakdowns, peaks, and outros
  when confidence is sufficient.
- Build several transition grammars: flow carryover, focal morph, depth travel,
  controlled fracture, and negative-space reveal.
- Select transitions from scene compatibility and current musical state.
- Make `N` request the next scene while automatic direction continues. Use a
  separate explicit control if a manual lock is ever added.
- Show a short, optional state label after manual input so auto, locked, scene,
  ASCII, and sync state are never ambiguous.

### Exit criteria

- High-confidence changes land on detected musical boundaries.
- No automatic sequence repeats a recent visual shape or energy character.
- Repeated song sections produce recognizable visual recurrence.
- Every scene has at least two strong incoming and outgoing transition paths.
- Manual input never leaves the user in an accidental permanent mode.

## Phase 5: v0.8 control and accessibility

Give users meaningful control without making configuration necessary.

### Work

- Persist favorites, hidden scenes, ASCII state, intensity, brightness, motion
  level, audio delay, display selection, and director profile.
- Add a small set of director profiles such as balanced, kinetic, restrained,
  and high contrast. Profiles change selection and intensity, not core timing.
- Add a first-run control reference and an optional minimal status overlay.
- Add reduced-motion and photosensitivity-conscious modes with explicit flash,
  luminance-change, and camera-motion limits.
- Add color-vision-safe palette constraints and high-contrast materials.
- Work with deaf and hard-of-hearing testers before making accessibility claims.
  Test whether rhythm roles and arrangement changes are understandable, then
  revise the mappings from their feedback.
- Version the preferences file and test migration between releases.

### Exit criteria

- Default behavior remains strong with no setup.
- Every preference has a clear visual effect and a safe default.
- Reduced-motion and high-contrast modes pass their measured limits.
- Accessibility wording reflects completed testing rather than assumptions.

## Phase 6: v0.9 authoring and curation

Allow more people to create scenes while keeping the official rotation strict.

### Work

- Define a versioned scene-pack format with shader files, metadata, declared
  musical roles, transition anchors, performance limits, and attribution.
- Build `omadrop pack validate` for compilation, missing metadata, unsafe
  resource use, response thresholds, and frame-time limits.
- Add an authoring session with shader reload, deterministic audio replay,
  signal inspection, and side-by-side continuous and ASCII output.
- Separate installed community packs from the curated official rotation.
- Require manual review before a community scene can be featured.
- Keep projectM as a compatibility and historical preset path, not the design
  constraint for native scenes.

### Exit criteria

- A third party can create and test a scene without modifying Omadrop source.
- Broken or incompatible packs fail with specific errors.
- Community content cannot silently enter the official automatic rotation.
- The scene API can evolve without breaking older validated packs.

## Phase 7: v1.0 release quality

### Work

- Complete long-duration stability, GPU compatibility, display hotplug, output
  switching, suspend and resume, and memory-growth testing.
- Ship reliable packages and updates for Omarchy first, followed by broader
  Wayland packaging only when behavior matches the Omarchy build.
- Add crash diagnostics that contain no captured audio or private track data.
- Finish the website gallery, scene documentation, keyboard reference, and
  troubleshooting path.
- Record a rights-cleared launch film that demonstrates quiet, rhythmic, dense,
  transitional, and peak passages.
- Review every scene again and remove anything below the final quality floor.

### Exit criteria

- At least 18 visually distinct native scenes pass every gate.
- The director produces coherent full-song performances across the replay set.
- Installation, launch, playback, display behavior, and shutdown have no known
  critical defects.
- Omadrop can run for hours without frame degradation, memory growth, or sync
  drift.

## Immediate work order

1. Add reviewed real, rights-cleared songs to the locked synthetic replay set
   and complete continuous-motion review for the established scene set.
2. Finish output-change, display-hotplug, shutdown, and long-run operational
   regressions needed to close Phase 0.
3. Normalize transient response and stabilize tempo, downbeat, and phrase
   detection across the replay set.
4. Add the developer signal monitor and repeatable presentation-delay tool.
5. Continue splitting track state, display session, cover presentation,
   direction, and rendering out of the live application loop.
6. Tune automatic direction across the complete 18-scene library and enforce
   visual-family spacing so consecutive scenes remain meaningfully different.
7. Promote only candidates that pass still-frame, motion, music-response,
   ASCII, transition, and performance review.

This order improves the existing product before increasing its surface area.
Each release should be obviously better in use, not only larger in a feature
list.

## Current progress

Completed after v0.3:

- Added a deterministic 32-second structured fixture and a ten-scene scorecard
  for transient response, motion coverage, recovery, and silence drift.
- Removed generic full-frame transient transforms from Spectral Ribbons and
  Constellation Field, then restored strong role-specific response inside each
  scene's own geometry.
- Added normalized chroma, rhythmic density, syncopation, tonal motion, and
  harmonic-change signals to the shared `MusicFrame` contract.
- Split settings, artwork, compatibility loading, and PipeWire capture out of
  the live application loop.
- Added a versioned scene registry for identity, shader, materials, musical
  roles, selection traits, transition anchors, and performance limits.
- Added an opening-audio correlation audit. Controlled demo recordings now
  fail before export when captured audio does not match the approved source.
- Added sparse, selective, and flow motion grammars with enforced continuous
  coverage and global-pulse limits, then removed generic full-frame pulsing
  from the selective scenes.
- Removed transient-driven full-frame feedback fading from all native scenes.
  Role response now stays inside each composition's own geometry. Added pulse
  duty limits so frequent frame-wide pumping cannot pass on response strength
  alone.
- Added configurable full-resolution song replay and an all-scene full-song
  gallery. The deterministic fixture and the current 30-second real-song
  review both pass all ten scene gates.
- Added a FIFO-backed 60 FPS replay encoder with source-matched audio for
  continuous motion review without desktop capture or temporary raw frames.
- Added five hash-locked, repository-synthesized replay profiles covering
  structured electronic, sparse acoustic-like, dense compressed, sustained
  vocal-like, and syncopated fast-section material. All fifty scene and profile
  combinations pass the enforced scorecard.
- Split Orbital Loom's percussion across distinct thread families and reduced
  synchronized snare luminance in Spectral Ribbons and Centrifuge after the
  expanded suite exposed pulse regressions that one song did not.
- Added true 1280x720 continuous and ASCII review galleries across six album
  colors and grayscale.
- Raised all ten current scenes to the still-frame quality floor. Time-sampled
  full-song review now passes; continuous multi-genre motion and the remaining
  operational regressions are still required before Phase 0 is complete.
- Added active-playback, pause, resume, seek, and track-change clock tests.
  Extracted paired-display transport from the live loop with atomic state,
  music, and focused-monitor control-request coverage.
- Added a two-display launcher harness that verifies readiness barriers,
  per-PID monitor routing, fullscreen placement, synchronized reveal, sibling
  shutdown, and runtime-file cleanup without opening real windows.
- Added runtime display-topology supervision. Connecting or removing a monitor
  now closes the old synchronized pair before revealing a newly routed set, and
  the launcher harness covers the two-display to one-display hotplug path.
- Extracted and tested the default-output handoff sequence. Sink changes now
  prove capture shutdown, analysis reset, per-output delay selection, and
  capture restart order, and capture startup detects an executable failure
  instead of silently leaving the visualizer without audio.
- Added an automated opening-audio guard test. A delayed approved source passes
  while an unrelated source is rejected, covering the final recording audit
  that protects release demos from stray desktop audio.
- Added a synchronized 1080p renderer soak across every scene and successor
  transition. The initial ten-minute, 36,000-frame run completed without an
  OpenGL error or slow-frame streak, held 0.71 ms at the 99th percentile on the
  reference machine, and grew resident memory by 3.4 MiB after warmup.
- Added role-specific overlap control to Spectral Ribbons after the continuity
  audit exposed a stacked-percussion jolt. Its worst-frame similarity improved
  from 0.63 to 0.80 while the real-song snare response remained above the
  required floor and all fifty synthetic replay combinations continued to pass.
- Separated transient normalization from continuous visual energy. Quiet,
  normal, loud, and compressed masters now produce the same deterministic
  kick, snare, and hat counts without making low-level texture drive constant
  motion. A locked noise-floor case prevents sensitivity changes from creating
  false percussion, and Orbital Loom now keeps kick changes local to its
  threads and aperture instead of repeatedly scaling the full composition.
- Locked tempo-ambiguity, breakdown, and syncopation regressions. The analyzer
  resolves a 70/140 half-time pattern to 139 BPM, holds its 120 BPM clock
  through a four-second onset-free gap, reacquires on the first returning beat,
  and remains aligned under displaced percussion.
- Made role separation and gesture recovery release gates instead of review
  notes. A scene now fails when kick, snare, and hat all move the same broad
  area, when their responses accumulate into constant activity, or when even a
  deliberate flow scene spends more than 60 percent of its frames in coherent
  full-frame motion.
- Added a replay-only signal monitor for deterministic tuning. It shows beat
  and bar phase, clock confidence, separate percussion roles, rhythmic density,
  and section novelty over the rendered scene, while the timeline records BPM
  and confidence numerically. Normal playback cannot enable the monitor.
- Extracted the final display compositor from the live application loop. Cover
  blending, continuous and ASCII material, backend transitions, per-preset
  reactions, and launch and shutdown visibility now cross one typed frame
  interface with an independent hidden-context OpenGL test.
- Extracted asynchronous MPRIS polling from the live loop. Track observations
  now arrive through a tested lifecycle that covers helper execution, partial
  output buffering, invalid state, no active player, duplicate starts, and
  shutdown cleanup, while `PlaybackClock` remains the sole owner of projected
  song position.
- Extracted cover timing into a deterministic presentation state machine with
  hold, dissolve, completion, synchronized start-gate restart, clear, and
  replacement tests. Track changes now remove the previous artwork before a
  new cover is loaded, preventing a missing cover from displaying the prior
  song's image.
- Removed hardcoded scene arrays from the gallery and scorecard tools. A
  validated registry-list executable now supplies canonical scene identity to
  every review path, and suite totals adapt automatically as the library grows.
- Added Negative Space as the first post-v0.3 scene. Its stable luminous field
  uses a dominant organic void, localized rim timing, a kick cutout, a snare
  incision, hat perforations, and a section cut instead of full-frame pulsing.
  Continuous color, grayscale, ASCII, six album palettes, the 55-combination
  replay suite, a real-song scorecard, and a ten-minute 1080p soak all pass.
- Added Ink Current as the first fluid post-v0.3 scene. Six compressed spectral
  zones reshape separate parts of one stable current; beats travel along it;
  kick eddies, snare cuts, hat droplets, and section branches remain local.
  Continuous color, grayscale, ASCII, six album palettes, the 60-combination
  replay suite, and a 1080p renderer soak all pass.
- Added Glass Choir as a restrained harmonic scene. Five fixed, overlapping
  shards expose moving internal caustics while beat travel, low-pane kick
  resonance, snare fractures, hat-tip glints, and section reassembly stay
  local. Continuous color, grayscale, ASCII, six album palettes, the
  65-combination replay suite, and a 1080p renderer soak all pass.
- Added Shadow Architecture as the first ray-marched deep scene. Three fixed,
  offset concrete portals preserve a stable silhouette and floor reflection;
  beats travel through the void while kicks, snares, hats, and sections light
  separate surfaces. Continuous color, grayscale, ASCII, six album palettes,
  the 70-combination replay suite, and a 1080p renderer soak pass without
  global pulse activity or a slow-frame streak.
- Added Particle Weave as a stable textile scene. Five braided bead strands
  retain their silhouette while a traveling beat shuttle, low kick knots, a
  six-bead snare stitch, selected hat beads, and one section thread respond in
  separate locations. Continuous color, grayscale, ASCII, six album palettes,
  the 75-combination replay suite, and a 3,600-frame 1080p renderer soak pass
  with zero pulse-duty activity or slow-frame streaks.
- Added Living Mosaic as a filled organic scene. Its fixed stained cells keep
  a stable full-frame composition while beat timing selects narrow seams and
  percussion activates separate cells, cuts, and nuclei. Continuous color,
  grayscale, high-exposure ASCII, six album palettes, the 80-combination
  replay suite, and a 3,600-frame 1080p soak pass with zero pulse-duty activity
  in every profile and no slow-frame streak.
- Added Lumen Fold as a quiet light-installation scene. Five fixed translucent
  sheets keep different lengths, widths, faces, and reflections while beat,
  kick, snare, hats, and section cues occupy separate surfaces. Continuous
  color, grayscale, high-exposure ASCII, six album palettes, the 85-combination
  replay suite, and a 3,600-frame 1080p soak pass with zero pulse-duty activity
  in every profile and no slow-frame streak.
- Added Paper Horizon as a representational cut-paper scene. Four fixed
  landscape layers hold still while a beat lantern, two kick-lit ridges, snare
  tears, hat stars, and the crescent carry separate cues. Continuous color,
  grayscale, high-exposure ASCII, six album palettes, the 90-combination replay
  suite, and a 3,600-frame 1080p soak pass with zero pulse-duty activity in
  every profile and no slow-frame streak.
- Added visual-family metadata to all 18 scenes. Automatic selection now adds
  a recency penalty for radial, filament, depth, vertical, landscape, network,
  minimal, fluid, faceted, and cellular repetition while manual scene requests
  and motif recall remain exact. Deterministic director tests cover radial and
  landscape spacing under audio that would otherwise favor repetition.
- Added a 128-second deterministic director program built from all five locked
  audio profiles. It runs through the production analyzer and renderer, locks
  the six-scene performance sequence, rejects adjacent family repetition, and
  enforces at least eight seconds between completed automatic scene changes.
  The first run exposed and fixed an immediate depth-family repeat and a
  premature motif recall after only 3.4 seconds.
- Added five native transition grammars selected from scene compatibility:
  flow carry, focal morph, depth travel, controlled fracture, and
  negative-space reveal. All 18 scenes now have at least two deterministic
  incoming and outgoing production-compositor paths. Multi-display state
  accepts every native mode, and a hidden harness checks both halves of every
  path while optionally rendering review frames without desktop contamination.
  Narrow spatial reveal boundaries preserve the identity of both scenes
  instead of holding a soft double exposure through the midpoint.
- Made transition selection structure-aware after compatibility is satisfied.
  Calm releases use a negative-space reveal, clear harmonic turns use a
  controlled fracture, rising or dense passages carry motion, and balanced
  passages use a focal morph. The director stores the selected grammar at the
  boundary and synchronizes it across displays, preventing momentary hits from
  changing transition behavior mid-flight. Deterministic visual sequences and
  the 128-second analyzer replay cover the new decisions.
- Replaced the one-off ASCII preference with an atomically written, versioned
  preferences file. It preserves ASCII mode and has validated fields for
  intensity, brightness, motion, reduced motion, high contrast, director
  profile, favorites, and hidden scenes. Existing `ascii-enabled` files migrate
  on first load, malformed values fall back or clamp safely, duplicate and
  invalid scene names are rejected, and a newer unknown format is never
  overwritten by an older build.
- Added synchronized live controls for local response intensity, brightness,
  ambient motion, reduced motion, and high contrast. Reduced motion caps slow
  feedback drift and transition travel without suppressing the separate kick,
  snare, hat, beat, and section cues. The two broad flow scenes show about 68
  percent less measured ambient movement under the reduced policy, while a
  separate renderer check proves intensity still changes a local gesture. All
  90 default scene/profile combinations still pass, as do separate 18-scene
  structured-track audits at 125 percent intensity and in reduced-motion mode.
  A 3,600-frame 1080p soak reports 0.40 ms at the 99th percentile, no slow
  streak, and 0.88 MiB resident-memory growth.
- Added a synchronized, self-contained status overlay for interactive changes.
  Scene skips explicitly show `AUTO: <scene>`, while ASCII, fullscreen, sync,
  intensity, brightness, ambient motion, reduced motion, and high contrast show
  their resulting state. The label fades after 1.8 seconds and is rendered
  after the scene compositor, so it cannot contaminate feedback or recordings
  without a user action. Hidden-context tests cover its raster, final pass,
  placement, expiry, and paired manual-scene cue.
- Activated the four persisted director profiles without changing scene timing.
  Balanced keeps the established score, Kinetic favors percussive and flow
  compositions, Restrained favors sparse lower-energy compositions, and High
  Contrast favors a different motion grammar and transition anchor from the
  current scene. Deterministic selection tests lock a distinct result for each
  policy where appropriate, track resets preserve the choice, full resets
  return to Balanced, and the selected profile synchronizes across displays.
- Added persistent scene favorites and hiding. Favorites provide a modest
  automatic-selection preference without overriding musical fit. Hidden scenes
  are removed from automatic direction, motif recall, opening selection, and
  manual `N`/`P` navigation. `F`, `X`, and `Shift+X` apply across paired
  displays, and the director rejects a hidden set that would leave fewer than
  two usable scenes.
- Tightened the scorecard's broad-pulse duty ceiling for flow scenes from 60
  to 25 percent. Spectral Ribbons now assigns kick, snare, and hat gestures to
  separate local windows and removes raw spectrum changes from its full-width
  geometry. Its broad-pulse duty fell from 44 to 55 percent to zero across all
  five locked replay profiles while every transient response remains above
  2.8 times quiet motion.
