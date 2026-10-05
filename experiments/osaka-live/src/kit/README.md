# Osaka piece kit

A piece is a drawing operation. It writes into the caller's Canvas or runs an
existing GPU pass at the caller's insertion point. A piece can share body and
light canvases with other pieces. It does not need its own texture.

A profile is a named version of a piece's behavior and defaults. This first
step uses compiled C++ types named `Osaka...V1`, with `osaka-*-v1` identifiers.
Their methods hold Osaka's existing constants and arithmetic. `DiscLook` and
`MountainLook` also expose the existing appearance parameters. The primitive
wrappers keep their old signatures and default arguments. These profiles are
fixed Osaka defaults, not a scene loader or a general SVG importer. A later
scene.json and SVG layer can instantiate these pieces after its own fidelity
checks.

## Where Osaka uses each piece

| Module | Profiles and Osaka mapping |
| --- | --- |
| `primitives.h` | Warm pane, dark pane, lattice and roof. Used by the downhill, near and right houses, and the deck. `palette.h` keeps the original shared colors. |
| `disc.h` | `osaka-disc-v1`: moon, veil, halo and bass rings, at the disc hook. The existing `drawDisc` API remains available. |
| `mountain.h` | `osaka-mountain-v1`: cone and optional snow cap, at the mountain hook. The existing `drawMountain` API remains available. |
| `haze.h` | `osaka-haze-v1`: the existing additive band pass and default noise size. Used by ridges, valley and downhill haze through `hazeBand`. |
| `sky.h` | `osaka-sky-v1`: sky gradient, clouds and stars, before the sky hook. |
| `ridges.h` | `osaka-ridges-v1`: the three background ridges and their haze passes. The unchanged `ridgeY` helper also serves the near ridge. |
| `town.h` | Separate profiles for the near shell, right shells 2 and 3, near deck, street surface, railing, cart frame, near mask and shamisen mask. Each appends its original retained span into the caller's original Canvas. |
| `pane.h` | `osaka-pane-v1`: scheduled panes, band lift, shared kick and outward shell flashes. It receives the existing shell list; it does not trigger fireworks. |
| `rooms.h` | Separate near-room, right-room-2, right-room-3 and cart-room profiles. The original pane layouts, light gradients, glow and cyan flicker stay with their room. `layout.h` holds the unchanged near and upper-right layout tables. |
| `neon.h` | `osaka-neon-v1`: board, retained tubes, broad glow, stutter and neon gain. The caller still adds bulbs and invokes `submit` on the shared neon Canvas at its original position. |
| `sign-outlines.cpp` | The unchanged `drawSignGlyph` implementation reads the existing outline asset. It serves the izakaya sign and cart curtains. |
| `lanterns.h`, `onset.h` | Paper, hand-held and cart lantern profiles. The original onset lookup reads Score without creating events. |
| `cloth.h` | Street reflected cloth and separate cart/izakaya curtain profiles. They write into their original shared body/light canvases. |
| `festoon.h`, `sky-lanterns.h` | Festoon and valley/couple releases. Existing positions, serial-based beat indexing, shell flashes and retirement times. |
| `animals.h`, `chime.h` | Moths, railing/sill/veranda cats and chime, including original masked-sill insertion and gestures. |
| `city.h`, `downhill.h` | Procedural city and downhill row profiles, retaining all layout/window RNG draws and cache keys. |
| `grass.h` | Grass and flowers share the caller's RNG across the original intervening body pass. |
| `wisteria.h` | Sorted canopy/raceme layout, prepared ellipses, spatial batching and the two shared petal-image passes. |
| `network.h`, `poles.h`, `strands.h`, `pulses.h` | Original pole/span data and attachments, pole art, strand body/hum and pulse streams. The far/near caller still owns Canvas allocation, retained insertion and final submission. |
| `flock.h` | Original landing/formation tables, flock rendering and per-frame dip profile shared by strands and pulses. |
| `events.h`, `light-wave.h`, `firework.h` | Versioned shell/life defaults and one central event input from the unchanged Schedule; the same state feeds panes, festoon, lanterns, birds and figures. Light-wave and star/smoke profiles preserve accumulation and pass order. |

## Keep the composition contract

`osaka.cpp` owns draw order, Canvas acquisition, parent profile groups, the
reflection snapshot and pass submission around the town pieces. Keep those
positions. Retained keys and camera parameters belong to the current Ctx;
these pieces do not introduce a new cache namespace or transform system.
Backdrop shader strings remain literal in the kit shader headers, with the
old shader header including them for remaining callers. Effect names, uniforms
and blend modes remain the same.

`osaka-legacy.h` now provides only a source-compatible include for round 1
profiles. The `OsakaLegacyLife` name aliases `OsakaEventState` from `events.h`; it owns no
second shell list, clock or detector. `OsakaEventsV1::at` is invoked at the same
composition entry points as the original `lifeAt`. All consumers of that draw
receive the same state. Schedule and Score, including the global event serial,
remain unchanged.

Procedural defaults stay pinned in their named `osaka-*-v1` profiles:
city RNG 41 (two layers, x 560..1400 and conditional roof advancement), downhill
RNG 55 (three row tables and uninterrupted window draws), grass RNG 66 (70
strokes then 54 flowers on the same stream), wisteria RNG 88 (15 crowns, 30
sorted racemes and original petal draws), strand RNG 101 plus quay RNG 1012+i,
and shell/star/smoke RNG 404+31*si / 900+si. All hash keys and jit inputs retain
their original values and evaluation order.

Actor choreography, near-ridge trees, train, reflections and remaining
composition stay in Osaka. Before data-loaded Osaka is complete, extract those
groups and make render insertion/reflection capture explicit. Do not add a
per-world executable extension API, file format or SVG importer in this step.

For an extraction, retain the previous binary and run both the looped music
and silent captures on the same GPU, seed, size and scale. Run
`tools/compare-frames.py --require-identical` against the previous commit, then
against the original baseline at the milestone. This gates every saved RGB
pixel and the complete 5 Hz brightness/clock/firework series. It does not check
every full-rate motion frame or establish artistic acceptance.

Round 2 also compares exact production PCM-hop poses selected from the entire
music/silent fixtures: full/small launch/burst flashes and finales, bird
approach/landing/scatter/return, six measured band lifts at onsets and late
lantern fades. An external capture-only main translation unit supplies those
saved poses and faster lossless PNG encoding; production main stays unchanged.
These images close the named cadence gaps, not all possible seeds, camera
travel or full-rate motion. The original-baseline comparison and same-device
60 s GPU profiles remain required at the milestone.
