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

## Keep the composition contract

`osaka.cpp` owns draw order, Canvas acquisition, parent profile groups, the
reflection snapshot and pass submission around the town pieces. Keep those
positions. Retained keys and camera parameters belong to the current Ctx;
these pieces do not introduce a new cache namespace or transform system.
Backdrop shader strings remain literal in the kit shader headers, with the
old shader header including them for remaining callers. Effect names, uniforms
and blend modes remain the same.

`osaka-legacy.h` is an internal bridge for the current Shell and Life values.
Firework planning, actors, wires, birds, foliage and the remaining composition
still live in Osaka. Before data-loaded Osaka is complete, replace that bridge
with shared event inputs, extract those remaining groups, and make render
insertion and reflection capture explicit in the composition. Do not add a
per-world executable extension API.

For an extraction, retain the previous binary and run both the looped music
and silent captures on the same GPU, seed, size and scale. Run
`tools/compare-frames.py --require-identical` against the previous commit, then
against the original baseline at the milestone. This gates every saved RGB
pixel and the complete 5 Hz brightness/clock/firework series. It does not check
every full-rate motion frame or establish artistic acceptance.
