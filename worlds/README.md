# World folders

`osaka-jade/scene.json` holds the ordered render stages, stable instance IDs,
gates, event references, placements and typed version 1 profile settings for
Osaka Jade. `schema/scene-v1.schema.json` describes the current format. The
loader also checks unique IDs and profile/piece and event dependencies.

The renderer validates and loads the folder once at startup. Missing or invalid
files are errors, with a filename, JSON path and expected value. There is no
compiled fallback. Installed worlds live in `/usr/lib/omadrop/worlds/`, beside
`bin/`. For a source build, set `OMADROP_WORLDS` to this directory. The default
world is `osaka-jade`; pass `--world` to load another folder:

```sh
OMADROP_WORLDS="$PWD/worlds" experiments/osaka-live/build/omadrop-osaka
OMADROP_WORLDS="$PWD/worlds" experiments/osaka-live/build/omadrop-osaka --world examples/lit-windows
```

Numbers use exact decimal doubles; existing float conversions happen at the
same drawing boundaries. Inside a profile block every field is required, and a
block you leave out keeps the library defaults. Instance IDs are unique across
stages, slots and explicit nodes; stage and slot array order is significant.
Version 1 accepts the Osaka world, the example worlds and their named library
profiles.

Art paths, shaders and procedural behavior remain in the kit. SVG validation,
scene art bindings and window label shorthand are available below. No general
SVG renderer, expressions or executable world code is supported today. Preview
mode (below) reloads a world while you draw.

## Supported SVG

SVG subset version 1 compiles artwork into the existing Canvas at world load.
The headless import/check tool is also available. Use a 1920 by 1080 artboard
with `viewBox="0 0 1920 1080"`.

Groups, paths (including curves and arcs), rectangles, circles, ellipses,
polygons, polylines and lines are supported. You can use transforms, solid
fills, linear/radial gradients, fill rules, opacity, and strokes with round,
butt or square caps and round, miter or bevel joins. Gradients support both
coordinate units, gradient transforms and up to 1024 stops, with pad spread.
Local defs, symbols and use references work; symbol fitting supports the default
centered fit or `preserveAspectRatio="none"`.

In Inkscape, outline text with Path > Object to Path and remove filters before
export. In Figma, convert text to vector outlines before SVG export. Keep stable
`id` values when exporting or editing. Keep an artist label in `inkscape:label`
or `data-name`; check these after export if the editor renames or strips them.

Text, images, filters, masks, clip paths, patterns, external references and CSS
stylesheets are rejected. Inline `style` attributes are supported. Other
unsupported elements or properties produce a filename, element ID and line
number so you can fix the export. Use plain numbers or px for shape dimensions;
percentages are supported for opacity, gradient coordinates and stop offsets.

## Make layers react to music

Windows are the simplest piece. Lanterns, lamps, neon signs, glows and wires
work the same way and are in the table further down.

### Windows

Name an SVG element with `window.band0`, replacing `band0` with the music band
you want from `band0` through `band5`. The label can also include `kick`,
`onset` or `always`:

```svg
<rect id="kitchen-window" data-name="window.band2.always.kick"
      x="120" y="160" width="90" height="120" fill="#10201b"/>
```

`band0` through `band5` are required. Exactly one is allowed. `kick` adds a
short bass-hit boost, `onset` adds a brief flash on a new sound, and `always`
keeps a steady warm level when the music is quiet. Tokens can appear in any
order after `window`. Use `inkscape:label` or `data-name` for the name and keep
the element's `id` unique.

`worlds/examples/lit-windows/` is a shorthand-only example with six windows.
Its `scene.json` has no window nodes:

```sh
OMADROP_WORLDS="$PWD/worlds" experiments/osaka-live/build/omadrop-osaka --world examples/lit-windows
```

Build and run the headless check tool from the repository root:

```sh
cmake -S experiments/osaka-live/tests -B experiments/osaka-live/build-tests
cmake --build experiments/osaka-live/build-tests --target omadrop-svg-import
experiments/osaka-live/build-tests/omadrop-svg-import --import-svg your-art.svg
```

It prints subset version 1 and each element's ID, label, type and source line as
JSON. Invalid art prints a diagnostic and exits with status 1. Artwork is bounded
to 16 MiB, 10,000 source elements, nesting depth 64, 20,000 compiled instances
and 2 million compiled vertices; extreme coordinates are rejected too.

### Lanterns, lamps, neon, glows and wires

Every label is a piece name followed by dot separated tokens, all lowercase,
with exactly one band from `band0` to `band5`. Tokens can come in any order.

| Label | Tokens | What it does |
| --- | --- | --- |
| `window` | `kick` `onset` `always` | Fills the shape with warm light that follows the band (see above). |
| `lantern` | `kick` `sway` | A paper lantern at the middle of the shape, as tall as the shape, with Osaka's warm paper look. Its glow follows the band. `kick` adds a flash on the bass, `sway` a gentle swing in the wind. The flat shape itself is not drawn, so use any placeholder shape. |
| `lamp` | `kick` | A soft pool of light centered on the shape, about three times as wide as it is. The light follows the band, and `kick` adds a flash on the bass. The shape stays as the fixture, so label the lamp head. |
| `neon` | `kick` `flicker` | Draws the shape in its own colors as a glowing neon tube with bloom. Brightness follows the band, and `kick` adds a flash on the bass. `flicker` adds Osaka's slow shimmer and the odd stutter. Outlined shapes (a stroke and no fill) look best. The flat shape itself is not drawn. |
| `glow` | `kick` `onset` `always` | A soft blurred glow of the shape, in the shape's colors, added on top of it. Good for signs, moons and crystals. It follows the band, and the tokens work as they do for windows. The shape itself stays visible. |
| `wire` | `pulse` | Draws a path as a thin dark wire with a pale upper edge, exactly along the path. A faint light hums along it with the band. `pulse` sends points of light travelling along it, faster and brighter as the band rises. The flat path itself is not drawn. |

Lanterns, lamps and glows need a shape (path, rect, circle, ellipse, line, polygon
or polyline), not a group. Neon and glow also need a shape that is not a `use`
reference and has no clip or group opacity. A mistake in a label stops the world
from loading and names the file, element id, label and the problem, for example
`art.svg: element id 'sign' label 'neon.band0.pulse': unknown token 'pulse'`.
A label that does not start with one of these names is plain art, so `lamp-post`
and `lanterns` are fine, but `lamp.post` is read as a lamp and is an error.
`omadrop-osaka --check` makes sure every labelled layer visibly responds.

`worlds/examples/night-street/` uses all of them: a lamp post, two lanterns, a
neon sign and two wires with pulses, plus windows. Its sky, moon, ridges and
haze come from `scene.json`.

## Backdrop pieces in scene.json

A world lists only the stages it draws, in render order (`Backdrop`, `Coast`,
`DistantTown`, `Foreground`, each at most once), and `slots` can be empty.
`events` can be left out and means `Life`. The `finish`, `disc`, `mountain`
and `profiles` blocks and each profile inside `profiles` can be left out too.
`disc` is needed when a `Disc` slot is used, and `mountain` when a `Mountain`
slot is. A block that is present must be complete, and anything unknown is an
error with the file, the JSON path and what was expected. The art is drawn at
the start of the `Foreground` stage, so the buildings in `art.svg` sit in front
of everything in the `Backdrop` stage.

| Piece | Where its settings go | What it draws |
| --- | --- | --- |
| `Sky` | `profiles["osaka-sky-v1"]`: `energyBase`, `energyBass`, `energySurge`, `timeOffset` | The night sky, clouds and stars. |
| `Disc` | `disc`: `x`, `y`, `parallax`, `radius`; colors and rings in `profiles["osaka-disc-v1"]` | The moon with its halo, and rings that travel out on strong bass hits. |
| `Mountain` | `mountain`: `x`, `parallax`, `peak`, `base`, `width`; colors in `profiles["osaka-mountain-v1"]` | A single mountain. |
| `Ridges` | The slot's `params.ridges`: a list of 1 to 8 ridges with `seed`, `base`, `amp`, `scale`, `parallax`, `top` and `bottom` (colors like `"#1d6a52"`). Leave `params` out for Osaka's three ridges. | Rolling hills with haze on each. |
| `Haze` | The slot's `params`: `y`, `sigma`, `lo`, `hi`, `color`, `gain`, and optionally `shift`, `drift` and `seed`. Noise size is in `profiles["osaka-haze-v1"]`. | A soft drifting band of mist across the picture. |
| `Firework` | None. The shells and their timing come from the schedule. | Fireworks bursts, in the `Chapter` gate. |

A slot is `{"id", "piece", "gate", "profile"}`, plus `params` where the table
says so. `gate` is usually `Always`; `Disc` and `Mountain` can use
`DiscEnabled` and `MountainEnabled`. `profile` is `osaka-sky-v1`,
`osaka-disc-v1`, `osaka-mountain-v1`, `osaka-ridges-v1`, `osaka-haze-v1` or
`osaka-firework-v1` for the matching piece. Here is the backdrop of
`examples/night-street`:

```json
{"id": "backdrop", "phase": "Backdrop", "slots": [
  {"id": "sky", "piece": "Sky", "gate": "Always", "profile": "osaka-sky-v1"},
  {"id": "moon", "piece": "Disc", "gate": "DiscEnabled", "profile": "osaka-disc-v1"},
  {"id": "hills", "piece": "Ridges", "gate": "Always", "profile": "osaka-ridges-v1",
   "params": {"ridges": [
     {"seed": 21, "base": 690, "amp": 190, "scale": 620, "parallax": 0.04, "top": "#082820", "bottom": "#185a46"}]}},
  {"id": "valley-haze", "piece": "Haze", "gate": "Always", "profile": "osaka-haze-v1",
   "params": {"y": 740, "sigma": 54, "lo": 0.2, "hi": 0.45, "drift": 5, "seed": 31, "color": "#5deba9", "gain": 0.14}}
]}
```

These pieces keep Osaka's drawing. A world can place and color them but cannot
change how they move.

## Preview while you draw

Open your world in a normal window and it updates every time you save:

```sh
OMADROP_WORLDS="$PWD/worlds" experiments/osaka-live/build/omadrop-osaka --preview --world examples/lit-windows
```

The window plays whatever your computer is playing. To use your own music
instead, add `--fixture FILE` (raw stereo float32, 44100 Hz); it loops. Edit
`scene.json` or `art.svg` in your editor and save. About a quarter of a second
later the picture changes, without a restart. Editors that save by writing a
temporary file and renaming it are fine.

A small note in the corner shows the world name, flashes "reloaded" after a good
save, and prints the problem in plain words if the files do not load. It names the
file and the place to fix, like the check command does. Until you save a version
that works, the last good picture keeps playing. Every reload result is also
written to the terminal. Press R to reload by hand and Esc to quit. The window
can be resized and keeps a 16:9 picture.

## Check your world

One command tells you whether a world is ready. It needs no window and no
sound card:

```sh
OMADROP_WORLDS="$PWD/worlds" experiments/osaka-live/build/omadrop-osaka --check --world examples/lit-windows
```

It prints PASS or FAIL for five checks and exits with status 0 only when every
required check passes:

1. **Loads.** The scene and artwork are valid. Errors name the file, element and
   line, the same as the SVG import tool.
2. **Opens the same way.** Two separate runs with the same seed and music draw
   identical frames. A failure means something changes between runs, such as the
   clock or an unseeded random value.
3. **Reacts to music.** The picture must differ from the same world in silence.
   If the world labels layers (`window`, `lantern`, `lamp`, `neon`, `glow` or
   `wire` with `band0` to `band5`), each of those layers must visibly respond
   too, and the ones that never do are listed.
4. **Frame budget.** The world's GPU time per 1080p frame, measured in the same
   run as Osaka Jade, must be no more than Osaka Jade plus 10%. Both numbers and
   the GPU name are printed. Under software rendering (for example
   `LIBGL_ALWAYS_SOFTWARE=1`) the numbers are only informational and do not
   decide the result.
5. **No harsh flashing.** The check renders the busiest 30 seconds of the music
   at 30 frames per second and applies the WCAG 2.3.1 flash rules. A flash is a
   pair of opposing brightness changes of at least 10% where the darker side is
   below 0.80 relative luminance. More than three flashes in one second over more
   than a quarter of the picture fails, and so does the same for saturated red.
   The worst second, when it happened and how much of the picture flashed are
   printed. WCAG measures area in a 10 degree field of view at normal viewing
   distance. This check simplifies that to a quarter of the whole frame, and it
   counts each pixel's own flashes, so it can miss flashes that move around the
   screen.

Options: `--fixture FILE` uses your own music (raw stereo float32, 44100 Hz).
Without it, a built-in 60 second test track is used. `--seconds N` sets how much
music is analyzed (default 30), `--seed N` the schedule seed (default 1), and
`--json OUT` writes the full report. The frame budget needs Osaka Jade: it is
found in the worlds folder, or give its folder with `--reference`.

`experiments/osaka-live/tests/worlds/` holds two deliberately bad worlds used by
the tests: `ignores-music` fails check 3 and `harsh-flashing` fails check 5.
They are not in `worlds/` and are not meant to be installed.

## Osaka static artwork

`osaka-jade/art.svg` is the source of truth for the static near/right shells,
roofs, lattices, deck, rail and cart frame, fixed sign glyphs and retained
masks. `scene.json.art` names the file and maps each piece binding to a drawable
SVG element ID. Keep those IDs while redrawing in Inkscape. Missing bindings,
missing IDs and elements incompatible with retained Canvas replay fail at
startup. The original retained keys, pass order and masks stay in the library.

The importer compiles native replay recipes once, preserving M/L/Q/C/Z commands,
round strokes, rectangle/circle operations and decimal float paints. SVG `use`,
viewport clipping and composited group opacity continue through the general
immutable `draw()` API; these cannot be bound to an existing retained span.
Animated geometry, procedural buildings and camera-dependent quay steps remain
compiled. See `experiments/osaka-live/src/kit/README.md` for the runtime boundary.

`python3 experiments/osaka-live/tools/export-osaka-art.py OUTPUT.svg` is a one-time
migration aid reading historical `bfc79ff`, not a build input. It records the
original commands and arithmetic, not flattened points. Do not regenerate over
artist edits. The seven sign outlines preserve the Noto Sans CJK JP Bold font
hash and SIL OFL provenance in the SVG; the bundled OFL notice remains required.

## Gradient sky

`GradientSky` uses `gradient-sky-v1` without changing `osaka-sky-v1`. Its slot
`params` contains `stops` (2..8 `{ "y": designPixel, "color": "#rrggbb" }`
entries, strictly increasing in 0..1080), `paperTop`, `paperBottom`,
`printGrade` (0..1 paper wash) and `grain` (0..0.1 static paper grain).
The ramp and wash come from the Journey prototype; there is no chapter clock.

The existing Disc profile optionally accepts `color2Hex` and `ringHex` (24-bit
integer paints) and `energyLift` (band-0 lift gain). Leave them out for the
original moon. These expose the prototype sunset disc without a separate sun
renderer; the usual halo, texture, veil and ring controls still apply.

The existing Mountain profile optionally accepts `snow` (0..1), `snowScale`
(positive) and `snowR`, `snowG`, `snowB`. These expose the original MountainLook
snow cap; omitted fields keep Osaka's snow-free silhouette and geometry.

## Living water

`WaterSurface` uses `water-surface-v1` with slot `params`. Optional fields:

| Control | Fields and bounds |
| --- | --- |
| Placement | `horizon` (0..1079), `nearY` (below horizon + 2, at most 1200), `x0`, `x1` (right of x0) |
| Density | `rows` (3..9), `textureRows` (0..65), `glints` (0..115), `sampleStep` (8..128 pixels) |
| Flow | `amplitude` (0..1), `wavelength` (0.5..4), `drift` (0..2), `phase`, `seed` (0..1000000) |
| Paint | `top`, `bottom`, `crest`, `texture`, `foam`, `underprint`, `glint`, `hotGlint` as hex colors; `opacity` (0..1) |
| Response | `bandGain` (0..3), `liftGain` (0..1), `kickGain` (0..0.5) |
| Print accents | `capDensity` (0..1), `capScale` (0..1.5), `glintX`, `glintDepth` (1..1080) |

Defaults are in `kit/water-surface.h`. The live depth planes, broken print
crests, texture and sunset reflections are ported from Journey's sea passes,
with a reduced geometry budget. Far-to-near rows use bands 5..0. Band-integrated
flow is continuous; measured level and lift brighten ripples and caps, while
kick adds a localized reflection shimmer. No crash, impact, spray or story clock.
The readiness check measures each depth row's crest and ripple area like the
label pieces' light areas. Hidden or unresponsive water rows fail music response.
