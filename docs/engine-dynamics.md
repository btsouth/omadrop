# Musical dynamics engine

The user can follow the prototype's music, but finds its motion insufficiently
smooth and its musical behavior insufficiently differentiated. This is an
engine requirement, not a request to add more scenes.

## Current listening rule: audible cause

The user accepted the causal direction and requested transfer to other scenes.
Constellation Field and Prism Garden now share the interface with Ink Current.
See [musical-scene-authoring.md](musical-scene-authoring.md) for the input
contract, authoring example, role assignments, registration, and validation.
The focused preview cycles through those three scenes only. Older descriptions
below that restrict the foundation to Ink Current are historical checkpoints.

The user accepted a brief settling response after sounds stop. Sustained sounds
should hold their visual state and change as the sound changes. No independent
drift, predicted beat gestures, or movement merely because time passes.
Role assignments may vary by scene but should remain understandable and
consistent. The user's usual listening is mostly rap, with no reference song
selected yet.

Ink Current now has fixed composition phases and color placement. Measured
frequency levels shape the folds; measured percussion estimates trigger their
accents and damped displacement. Independent flow, inferred beat/anticipation,
section-change gestures, and overall energy-driven breathing are removed from
this preview. The earlier groove/energy architecture below remains available
to the renderer but is not used by this scene.

The updated GPU contract measures zero motion in silence and effectively zero
motion for held audio measurements. Advancing scene time and changing inferred
beat/section signals cannot alter the image. A kick leaves a brief settling
response and recovers within 0.5 seconds; all six role-response checks pass.
Evidence: `cache/causal-motion/contract.log`.

These checks establish animation causality for supplied measurements. They do
not establish that the analyzer correctly distinguishes instruments in mixed
rap recordings. That remains the next listening and analysis validation task.

## Architecture

Audio capture feeds fixed 60 Hz analysis. That analysis describes estimated
percussion, six frequency roles, 32 spectral bands, groove, energy, and musical
structure. A separate presentation layer turns those measurements into motion
with continuous position and velocity. Scenes assign that motion to authored
parts of their geometry and material. Presentation follows the display clock.

The analyzer does not separate a mixed recording into perfectly identified
instruments. Its role estimates must be judged against real music, including
vocals, sustained bass, acoustic instrumentation, and dense masters.

## Current foundation

- `MusicalMotion` provides critically damped motion for independent impact,
  frequency, spectrum, groove, and energy channels. Its analytic integration
  uses elapsed time, not a fixed amount per rendered frame.
- Impact scaling preserves headroom between routine and strong hits. An
  immediate material accent accompanies the physical geometry response.
- Ink Current assigns bass chiefly to larger rear folds, midrange and snare to
  middle folds, and treble to finer front folds. Spatial regions also differ.
- Sustained frequency changes develop the material continuously between attacks.
- Ink Current's preview opts into display-rate rendering with one pacing
  authority and sub-millisecond time measurements. Its short framebuffer
  persistence is adjusted by elapsed time. Legacy scenes remain at their
  authored 60 Hz rate until their feedback equations are migrated.
- Audio analysis and synthetic/silent fallback advance on an independent
  60 Hz clock, regardless of the display refresh rate.
- Development diagnostics report actual application frame intervals and audio
  time. GPU render cost alone is not evidence of smooth presentation.

## Work sequence

1. Establish stable presentation and continuous, independent musical motion in
   the existing listening prototype. Compare 60, 144, and 165 Hz. Keep startup,
   shutdown, and paired transport working.
2. Validate the analyzer's role and strength estimates against a broader music
   corpus. Address missed/false attacks, compression, sustained material,
   perceived loudness, and calibration before claiming musical accuracy.
3. Author scene behavior around musical relationships: foreground/background,
   bass and melody counterpoint, buildup and release, and structural changes.
   Expand to another scene only when the first behavior is accepted live.
4. Migrate accepted scenes to elapsed-time feedback and the shared motion layer.
   Judge both the whole performance and isolated role behavior. A scene count
   and passing signal tests do not establish aesthetic quality.

## Acceptance

- Regular display cadence, including while analysis receives audio in packets.
- Attacks remain legible; body motion has continuity and controlled recovery.
- Different sounds visibly affect different structures and spatial scales.
- Stronger sounds produce appropriately larger responses without clipping the
  composition or reducing ordinary music to a constant maximum.
- Silence settles, sustained passages stay legible, and peaks have headroom.
- The live listening experience determines whether this direction succeeds.

## Validation on 2026-09-04

Hidden real-backend runs measured mean application intervals of 16.667 ms at
60 Hz, 6.944 ms at 144 Hz, and 6.061 ms at 165 Hz. Stable-window p95 intervals
were at most 16.681, 6.962, and 6.075 ms respectively. Synthetic analysis
advanced approximately six seconds in six seconds at every rate. This tests
application cadence, not visible compositor delivery or subjective smoothness.
Evidence and the repeatable harness are in `cache/musical-motion/`.

The motion unit test verifies elapsed-time integration, continuity, independent
roles, strength headroom, and recovery. The hidden GPU music-connection test
passes isolated and sustained role checks, silence, recovery, and reduced
motion. Its 1080p render cost measured 0.595 ms on this machine. All 18 native
scenes also pass the existing renderer contract.

The full quick check is **not green**. Its transition/policy test stops at
reduced-motion cue retention for Ink Current's hat response: 0.637 versus the
required 0.85, with spatial similarity 0.425 versus 0.55. Repeating with the
previous committed shader also fails (0.548 retention and 0.206 similarity).
This is a remaining prototype release blocker; the assertions are unchanged.
Preserve legible treble cues while reducing movement before claiming these
policies meet their contract.

The installed hung-query/stubborn-capture smoke still passes: ready frame
0.657 seconds, normal exit at 2.313 seconds with a 1.5-second auto-close.
All 44 installed runtime files match. The hidden GPU probe reports
`native_renderer=ok`; existing launch and close bindings remain configured.
