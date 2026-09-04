# Music-connected preview

The user's live review rejected the library as boring, stale, and insufficiently
reactive. This supersedes the old aesthetic requirement to keep compositions
fixed and restrict nearly every response to tiny local details.

## Plan

1. Rebuild Ink Current as flowing, shaded sheets with depth and evolving folds.
2. Map bass to large deformations, midrange to the body, snare to traveling
   creases, and treble to fine surface detail. Keep the existing analyzer and
   synchronized audio transport.
3. Inspect deterministic real-song renders and direct isolated-response probes.
   Measure silence, immediate response, recovery, reduced motion, and GPU cost.
   Existing motion-coverage ceilings are diagnostic for this prototype. Do not
   change thresholds to claim a pass or confuse metrics with aesthetic success.
4. Install a reversible preview entry point. Super + Shift + V starts this
   scene in continuous rendering and holds it for listening. A compares ASCII;
   N/P allow deliberate comparisons. Preserve the normal launcher.
5. The user's music and live judgment determine whether to extend this direction
   to other scenes. This is a prototype, not a v1 release candidate.

## Acceptance

- Audible attacks visibly change shape immediately.
- Bass, midrange, and treble have distinguishable visual effects.
- Several seconds of music develop the composition instead of repeating a pose.
- Quiet passages settle; dense passages have room to expand.
- Continuous and ASCII materials preserve a legible subject.
- The preview uses the ordinary renderer, audio path, and display synchronization.
- Final aesthetic acceptance requires the user's listening test.

## Preview behavior

`bin/omadrop-preview` uses the ordinary launcher with Ink Current selected,
automatic scene changes held, and continuous rendering on entry. The cover
holds for one second and dissolves over two seconds. First-run instructions
are hidden for this listening test. A, N/P, and Esc remain available.

The installer adds `omadrop-preview` alongside `omadrop`; it does not change
the standard shortcut to preview mode. This workstation's shortcut is changed
separately with a backup. Restore the command in its existing binding to
`/home/bts/.local/bin/omadrop` to return to the ordinary automatic launch.

Reproduce the focused checks after running the normal build:

```bash
experiments/projectm-ascii/native-music-connection-test \
  shaders/native cache/music-connected-preview/probes
experiments/projectm-ascii/native-renderer-test shaders/native
```

The first probe uses identical clock and framebuffer histories for every audio
comparison. It checks first-frame response, sustained response, role separation,
recovery, finite bounded pixels, silence, reduced motion, and 1080p frame cost.

## Installed checkpoint, 2026-09-04

- Complete build and user-local installation succeeded. All 43 installed
  runtime files match the checkout. GPU doctor reports `native_renderer=ok`.
- The final native renderer audit passes all 18 scenes, including immediate
  percussion response, sustained response, continuity, and performance.
- The new connection probe passes. Mean first-frame RGB changes are 0.0566
  for kick, 0.0421 for snare, and 0.0109 for hat. All three return to baseline
  after the input stops. These are renderer measurements, not end-to-end
  audio latency or a subjective quality score.
- Sustained bass, middle, and treble produce distinct, persistent changes.
  Active motion measures about 41 times settled silence; reduced motion is
  about 30 percent of the normal value. The 1080p shader averages 0.50 ms on
  this RTX 4070 SUPER, excluding capture and final display composition.
- Preview hold survives 100 simulated seconds of section boundaries; manual
  next still works. Director and launcher synchronization/hotplug tests pass.
- Both complete approved tracks rendered successfully during development.
  Kick responses were 16.6 and 23.2 times quiet motion. Their broad-kick metric
  is about 0.36, exceeding the preceding release ceiling of 0.24. That ceiling
  was not relaxed. This remains a listening prototype rather than a passing
  full release candidate. Final interpolation smoothing was subsequently
  covered by the renderer and connection probes.
- Continuous and ASCII reference images, real-song motion sequences, raw
  probes, and logs are under `cache/music-connected-preview/`.
- The installed shortcut selects the preview and Hyprland reports no config
  errors. Its original configuration backup path is recorded in
  `cache/music-connected-preview/binding-backup.txt`.
- No visible desktop session was opened for testing. An SDL offscreen live
  smoke attempt exited before initialization; hidden OpenGL renderer probes
  passed. Actual listening, input focus, and paired-display presentation still
  need the user's next live test.

Do not reinstate the old sparse composition in response to its motion score.
Evaluate whether the larger deformations feel connected to the sound, then
revise the aesthetic acceptance criteria with the user.
