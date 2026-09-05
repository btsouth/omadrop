# V1 collection plan

## Product contract

V1 requires at least **20 distinct, accepted, high-quality themes**. The longer
term collection target is 50. Recolors and small geometry variations do not
count. The existing library and causal prototypes are not accepted release
art. Current accepted count: **0 / 20**.

The collection should remain worth watching for ten-minute stretches across
songs, primarily rap for the first listening reviews. That is a human review
requirement, not a claim that a stability test can prove entertainment.

Every visible change needs a measured audible cause. A sustained sound holds
its shape and changes with the sound. A hit may settle briefly, within roughly
half a second. No drifting camera, predicted beat animation, automatic orbit,
noise scroll, or continuous rotation to disguise weak art direction.

## Latest diagnostic direction

The user still finds the candidates repetitive, zoom-like or twitchy, and too
restrained in places. Follow [motion-review.md](motion-review.md) for the new
synchronized measurement workflow and controlled comparisons. No candidate
has passed visual or listening acceptance. Prioritize diagnosing this before
adding more themes.

## Execution order

1. **Three visual candidates.** Replace the technical-demo art with a sculptural
   material study, a landscape with depth, and a layered graphic/textile scene.
   Keep the shared causal interface. Inspect rendered output at quiet, sustained,
   and impact states before installation. These remain candidates until the
   user approves both appearance and musical behavior.
2. **Musical calibration.** Review sparse 808 rap, dense percussion, rapid hats,
   vocals with little accompaniment, sustained bass, and hard pauses. Measure
   false percussion cues and leakage between roles. Preserve direct response;
   do not compensate for detection errors with unrelated animation. Frequency
   bands are not vocal or instrument stems. Add channel-specific calibration
   only where the evidence supports it.
3. **Long listening and production foundation.** Validate ten-minute sequences,
   transitions across songs, two displays, startup, close, focus, reduced motion,
   scaling, and hardware frame pacing. Reconcile old tests that demand invented
   beats with the causal specification. Keep true accessibility failures open.
4. **Build in batches of four or five.** Review each batch for musical clarity,
   visual distinction, and sustained interest. Revise or discard weak scenes.
   No new batch should be justified by a target scene count alone.
5. **V1 release gate.** All 20 have individual visual/listening acceptance,
   causal tests, performance evidence, and supported policy behavior. Complete
   package/install, real dual-display and session lifecycle checks. Publishing
   remains user-owned. No release based solely on automated green checks.
6. **Grow to 50.** Expand the proven authoring system into new visual families
   and separately version a compatible third-party scene-pack interface.

## Collection coverage

These are design briefs, not implemented or accepted themes. Names may change.

| Slot | Working direction | Distinct composition/material | Musical organization |
| --- | --- | --- | --- |
| 1 | Opal Bloom | Solid pearlescent folded sculpture | Bass body, middle folds, bright fine ridges |
| 2 | Ember Atlas | Wide terrain of illuminated contours | Bass terrain, middle strata, fine crest detail |
| 3 | Chromatic Pleats | Overlapping silk/foil fans | Bass broad pleats, middle seams, treble fibers |
| 4 | Glass Estuary | Refractive branching channels | Low channel widths, middle branches, high caustics |
| 5 | Ceramic Assembly | Clustered glazed interlocking forms | Separate material bodies assigned to roles |
| 6 | Velvet Eclipse | Occluded light and dark sculptural discs | Bass occlusion, middle corona, high edge detail |
| 7 | Porcelain Reef | Branching organic mineral formations | Low trunks, middle branches, high polyps |
| 8 | Chrome Tension | Taut metal membranes and cable geometry | Low tension, middle curvature, high seams |
| 9 | Luminous Manuscript | Dense expressive typographic strokes | Low stroke mass, middle contours, high marks |
| 10 | Mineral Cross-section | Intricate cut-stone bands | Low boundaries, middle layers, high inclusions |
| 11 | Solar Filaments | Volumetric looking branching light | Low core, middle tendrils, high microfilaments |
| 12 | Paper Metropolis | Sculpted architectural relief, no tunnel | Low structures, middle panels, high windows |
| 13 | Ink Archipelago | Pigment pools and detailed fluid boundaries | Low pools, middle joins, high stippling |
| 14 | Woven Spectrum | Interlaced textile with depth and shadows | Separate directional frequency threads |
| 15 | Prism Chamber | Finite refractive space, no forward flight | Low planes, middle refraction, high dispersion |
| 16 | Copper Botanica | Metallic leaf clusters | Low stems, middle leaves, high veins |
| 17 | Electric Calligraphy | Spatially composed luminous strokes | Low main strokes, middle flourishes, high texture |
| 18 | Obsidian Fracture | Heavy polished rock and lit fractures | Low plates, middle gaps, high mineral sparks |
| 19 | Mosaic Relief | Deep tessellated sculptural wall | Low tile groups, middle faces, high inlays |
| 20 | Polar Lace | Fine crystalline layered branching | Low branches, middle fans, high lace |

No family receives credit for a renamed version of an earlier composition.
There should be variation in silhouette, depth, scale, density, material, and
sound mapping, not merely palette.

## Acceptance sheet for every theme

- **Visual:** compelling still composition; material depth; intentional light;
  detail at multiple scales; no clipping, alias shimmer, or generic primitives.
- **Musical:** each main role has a readable home; strong hits have headroom;
  sustained passages remain legible; pauses settle; no invented motion.
- **Range:** sparse, dense, quiet, and loud inputs produce meaningfully different
  states without washing out the composition or hiding ordinary details.
- **Duration:** ten-minute listening review across multiple tracks. Record where
  interest or correspondence fails, rather than claiming that activity equals
  interest. User verdict required.
- **Technical:** causal contract, bounded output, consistent display-rate motion,
  sustained runtime, quick launch/close, and both displays behaving correctly.
- **Policies:** reduced motion retains meaningful cues; flash limiting and color
  policies work. No unsupported accessibility claims.

Status progression: brief -> implemented candidate -> technical checks ->
visual review -> long listening review -> accepted. A failed aesthetic review
returns the theme to revision regardless of technical scores.

## First batch implementation notes

The existing three preview slots are reused to avoid expanding a rejected
library. Their stable IDs remain `constellation-field`, `prism-garden`, and
`ink-current` for compatibility. Display names and art change to the three
candidate directions. The normal native registry still contains 18 slots;
that is not the accepted theme count. N/P stays within this focused batch.

## Batch 1 checkpoint, 2026-09-04

Implemented and installed: Opal Bloom, Ember Atlas, Chromatic Pleats. All use
the shared musical interface and contain no scene clock or predicted rhythm.
Rendered quiet, individual-role, and held-input frames were inspected. A rough
terrain contour issue was corrected by refining ray/surface intersections.

All three causal contracts pass: distinct role response, sustained response,
stillness for held audio and silence, immunity to inferred beat/section changes,
and recovery after hits. Each completed its own 36,000-frame, simulated
ten-minute run at 1920x1080. P99 render times were 0.793, 0.716, and 0.698 ms;
measured resident-memory growth was about 0.41 MiB per run. These accelerated
render tests do not substitute for ten-minute human listening sessions.

Hidden installed runs held approximately 6.944 ms application intervals at
144 Hz and 6.061 ms at 165 Hz, with the correct audio clock. The installed
hung-query/stubborn-capture smoke reached a ready frame in 0.757 seconds and
closed in 2.431 seconds with a 1.5-second auto-close request. All 45 installed
runtime files match the checkout, and the GPU probe reports `native_renderer=ok`.
Motion, paired transport, and paired synchronization tests pass.

Three 12-second audio-bearing review clips were rendered from the existing
approved local music fixture. This is not broad rap validation. Source frames
and movie frames were inspected, exposing a review-export mismatch: the offline
writer applies gamma mapping that the live compositor does not apply in the
same way. Unify review output with the live pipeline before treating exported
clips as authoritative visual evidence. Live preview is the review target now.

Evidence: `cache/collection-batch1/`. Repeat a scene-specific stability test:

```sh
experiments/projectm-ascii/native-renderer-soak shaders/native 10 constellation-field
```

Remaining before the batch can be accepted: user visual review, ten-minute
multi-track listening, rap-specific role accuracy, actual dual-display pacing,
policy cue retention, and reconciliation of legacy scene metadata/tests with
the causal contract. The full release suite has not been certified green.
No candidate is counted as accepted. Next action is to revise the weakest
visual or musical aspect revealed by the listening review, then start the
next distinct family once this batch establishes a worthwhile quality level.
