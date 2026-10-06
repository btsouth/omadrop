# Osaka piece kit

A piece is a drawing operation. It writes into the caller's Canvas or runs an
existing GPU pass at the caller's insertion point. A piece can share body and
light canvases with other pieces. It does not need its own texture.

A profile is a named version of a piece's behavior and defaults, with stable
`osaka-*-v1` identifiers and typed `Osaka...V1` implementations. The world folder
now supplies ordered instances and several numeric profile settings; library
methods preserve the existing arithmetic and drawing behavior. `DiscLook` and
`MountainLook` expose appearance parameters, and primitive wrappers keep their
existing signatures. SVG art import and native retained-span replay are available.

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
| `sign-outlines.cpp` | The `drawSignGlyph` wrapper places and tints the seven SVG glyphs. It serves the izakaya sign and cart curtains. |
| `lanterns.h`, `onset.h` | Paper, hand-held and cart lantern profiles. The original onset lookup reads Score without creating events. |
| `cloth.h` | Street reflected cloth and separate cart/izakaya curtain profiles. They write into their original shared body/light canvases. |
| `festoon.h`, `sky-lanterns.h` | Festoon and valley/couple releases. Existing positions, serial-based beat indexing, shell flashes and retirement times. |
| `generic-window.h` | `generic-window-v1`: shorthand window fills in the main canvas pass. |
| `animals.h`, `chime.h` | Moths, railing/sill/veranda cats and chime, including original masked-sill insertion and gestures. |
| `city.h`, `downhill.h` | Procedural city and downhill row profiles, retaining all layout/window RNG draws and cache keys. |
| `grass.h` | Grass and flowers share the caller's RNG across the original intervening body pass. |
| `wisteria.h` | Sorted canopy/raceme layout, prepared ellipses, spatial batching and the two shared petal-image passes. |
| `network.h`, `poles.h`, `strands.h`, `pulses.h` | Original pole/span data and attachments, pole art, strand body/hum and pulse streams. The far/near caller still owns Canvas allocation, retained insertion and final submission. |
| `flock.h` | Original landing/formation tables, flock rendering and per-frame dip profile shared by strands and pulses. |
| `events.h`, `light-wave.h`, `firework.h` | Versioned shell/life defaults and one central event input from the unchanged Schedule; the same state feeds panes, festoon, lanterns, birds and figures. Light-wave and star/smoke profiles preserve accumulation and pass order. |

## Final compiled composition

`osaka.cpp` is the Osaka world description: four ordered typed render-slot
arrays, the shared life/flock event references, disc/mountain placement,
and the finish profile. `composition.cpp` executes the arrays using the
existing Canvas/Gpu renderer. `groups.cpp` keeps the original shared canvases,
retained keys and pass recipes, including masked shadows and reflection
capture after wires but before foreground people. Pieces do not acquire an
independent texture merely because they are library instances.

`actors.h` adds woman/fan, tea/kettle, patrons/cups, cook/ladle/pot,
customer/bowl, couple/lantern, child, lantern-bearer, cyclist/bicycle/headlamp
and shamisen/instrument profiles. `figure.h` accepts today's absolute design
pixel joint targets through the unchanged rig solver. Literal four-knot
windows are declarative `ActionWindow` clips; dynamic event-relative knots,
trigonometric tracks, gait and reach constraints retain their exact arithmetic.
`groups.h` also adds the near wooded ridge, train, shooting star, reflection, steam,
fog and glow profiles, plus town/cart/near-house/network render groups.

The temporary `osaka-legacy.h` bridge and `OsakaLegacyLife` alias are removed.
Every consumer uses `OsakaEventState`; each original composition entry point
still evaluates the same event state at the same time. Score, Schedule and the
global event serial are unchanged. Journey's existing hook APIs remain as
compatibility ports in the library executor, not arbitrary world callbacks.

Procedural seeds, RNG advancement, hash/jit keys, parallax and retained cache
keys remain the original v1 defaults. The grass and flower passes still share
RNG 66 across the intervening body pass. City/downhill/wisteria/strand and
shell/star/smoke streams retain their original ordering and values.

`effects_shaders.h` holds the unchanged reflection, steam, radial-light and fog
shader literals. The root shader header is only a compatibility include.

## Profile identifiers

These are the actual compiled profile names, grouped by header. An instance
uses its typed profile; the world render slots label that profile explicitly.

| Header | Immutable profile names |
| --- | --- |
| `actors.h` | `osaka-woman-fan-v1`, `osaka-tea-v1`, `osaka-patrons-v1`, `osaka-cook-v1`, `osaka-customer-v1`, `osaka-couple-v1`, `osaka-child-v1`, `osaka-bearer-v1`, `osaka-cyclist-v1`, `osaka-shamisen-v1` |
| `animals.h` | `osaka-moths-v1`, `osaka-rail-cat-v1`, `osaka-sill-cat-v1`, `osaka-veranda-cat-v1` |
| `chime.h` | `osaka-chime-v1` |
| `city.h` | `osaka-valley-city-v1` |
| `cloth.h` | `osaka-street-cloth-v1`, `osaka-noren-v1`, `osaka-izakaya-cloth-v1` |
| `composition.h` | `osaka-finish-v1`, `osaka-world-v1`, `osaka-composition-v1` |
| `disc.h` | `osaka-disc-v1` |
| `downhill.h` | `osaka-downhill-rows-v1` |
| `events.h` | `osaka-shell-v1`, `osaka-life-v1`, `osaka-events-v1` |
| `festoon.h` | `osaka-festoon-v1` |
| `figure.h` | `osaka-figure-v1` |
| `firework.h` | `osaka-firework-v1` |
| `flock.h` | `osaka-flock-v1`, `osaka-flock-dip-v1` |
| `grass.h` | `osaka-grass-v1`, `osaka-grass-flowers-v1` |
| `groups.h` | `osaka-near-ridge-v1`, `osaka-train-v1`, `osaka-reflection-v1`, `osaka-shooting-star-v1`, `osaka-fog-v1`, `osaka-glow-through-v1`, `osaka-steam-v1`, `osaka-right-town-v1`, `osaka-street-surface-group-v1`, `osaka-street-actors-v1`, `osaka-cart-group-v1`, `osaka-near-group-v1`, `osaka-network-group-v1` |
| `haze.h` | `osaka-haze-v1` |
| `generic-window.h` | `generic-window-v1` |
| `lanterns.h` | `osaka-paper-lantern-v1`, `osaka-lantern-v1`, `osaka-cart-lantern-v1` |
| `light-wave.h` | `osaka-light-wave-v1`, `osaka-festoon-light-wave-v1` |
| `mountain.h` | `osaka-mountain-v1` |
| `neon.h` | `osaka-neon-v1` |
| `network.h` | `osaka-wire-network-v1` |
| `pane.h` | `osaka-pane-v1` |
| `poles.h` | `osaka-poles-v1` |
| `primitives.h` | `osaka-warm-pane-v1`, `osaka-dark-pane-v1`, `osaka-lattice-v1`, `osaka-roof-v1` |
| `pulses.h` | `osaka-pulse-stream-v1` |
| `ridges.h` | `osaka-ridges-v1` |
| `rooms.h` | `osaka-near-room-v1`, `osaka-right-room-2-v1`, `osaka-right-room-3-v1`, `osaka-cart-room-v1` |
| `sky-lanterns.h` | `osaka-sky-lanterns-v1`, `osaka-couple-lantern-v1` |
| `sky.h` | `osaka-sky-v1` |
| `strands.h` | `osaka-strands-v1` |
| `town.h` | `osaka-near-house-v1`, `osaka-right-house-2-v1`, `osaka-right-house-3-v1`, `osaka-near-house-deck-v1`, `osaka-street-surface-v1`, `osaka-street-railing-v1`, `osaka-yatai-frame-v1`, `osaka-near-house-mask-v1`, `osaka-shamisen-mask-v1` |
| `wisteria.h` | `osaka-wisteria-v1` |

## World folders, step 2 round A

Osaka loads `worlds/osaka-jade/scene.json` once at startup through Qt JSON.
`world-loader.cpp` builds an owned immutable description, rejects unknown
fields and profiles, duplicate IDs, missing dependencies and non-finite values,
and reports the file, JSON path and expectation. The compiled description is
only in `tests/osaka-oracle.cpp`. See `worlds/README.md` for discovery and schema.

Now data: ordered stages/slots, gates, event refs, placements and finish; disc
colors, look defaults and ring response/timing coefficients; mountain colors,
sampling and curve coefficients; haze noise/cull settings; sky response gains
and time offset; pane response gains and near/upper/right pane layout tables;
neon board/tube/glow geometry, colors, glyph count/spacing, stutter, response and
pass gains. `parameters.h` defines their typed versioned shapes. Loader fields
are all required; compiled defaults support existing library callers and the
test oracle, rather than a runtime fallback for missing world data.

Still behavior: disc ring selection, shader/noise equations and uniforms;
mountain curve evaluator and snow-cap paths; haze/sky shaders and pass bounds;
pane recurrence and shared light-wave evaluator; room/primitive drawing and
actor clips; animated sign placement and tint. These require the later supported
art subset or typed track/constraint/pass contracts. Other library profiles
(actors, animals, events, groups, network, cloth, lanterns, vegetation, city,
ridges and downhill generators) remain compiled. Window layer shorthand
expands into `generic-window-v1` nodes. No generic expressions, new renderer or
event detector is added in this round.

Numeric substitutions preserve math order, original decimal doubles, RNG
keys, cache keys and the points where values narrow to floats. Art paths and
shaders retain their bytes. A validated description stays const for its entire
rendering lifetime; there is no per-frame JSON parsing or profile lookup.

## Verification contract

For each local extraction commit, compare both the 180 s looped music and
60 s silence fixtures with `compare-frames.py --require-identical`, seed 1,
1920x1080 and scale 1 on the same Intel GPU, against the immediately previous
commit. Compare the complete saved RGB/clock/firework series and selected
production PCM-hop poses, including actor clip phases and shell-relative
child/lantern phases. The final milestone also compares original `9f2b6d3`.
This is saved-frame fidelity, not every motion frame or artistic acceptance.

Final validation includes Osaka CTests, controller fake-process tests, and
interleaved same-device baseline/final 60 s GPU profiles with matching
render-thread CPU timing. Report shared-machine load for each run; timing
under changing load is a paired measurement, not a clean-device budget claim.

## Static art, step 2 round B2

`scene.json.art` binds the static town artwork to IDs in `art.svg`. The world
loader owns the imported art; `world-art.h` replays validated, immutable recipes
into the original caller-owned retained canvases. It preserves cache keys,
append points and pass grouping. Paths retain quadratics and explicit closes;
round strokes use Canvas tessellation, and decimal SVG RGB percentages narrow
directly to float instead of passing through QColor's 16-bit channels. No
per-frame XML/path/style parsing is performed. The existing `SvgArt::draw()` API
keeps the broader SVG subset and its committed goldens. `replay()` rejects
composited group opacity, use instances and viewport clipping rather than
changing their semantics. Scene bindings validate that compatibility at startup.

Town wrappers no longer construct the migrated outlines. `primitives.cpp`
still serves procedural downhill buildings and dynamic library consumers.
Camera-dependent quay clipping/steps, rooms, actors, cloth, neon response,
reflection snapshot timing and RNG streams remain compiled and unchanged.
Seven reusable sign paths now live in SVG, preserving the OFL notice/provenance;
`drawSignGlyph` applies the original animated placement, size, tint and alpha.
The exporter reads only the historical migration revision and is not used at
build or runtime. Edit SVG paths in place while preserving bound IDs.
