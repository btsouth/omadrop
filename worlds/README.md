# World folders

`osaka-jade/scene.json` holds the ordered render stages, stable instance IDs,
gates, event references, placements and typed version 1 profile settings for
Osaka Jade. `schema/scene-v1.schema.json` describes the current format. The
loader also checks unique IDs and profile/piece and event dependencies.

The renderer validates and loads the folder once at startup. Missing or invalid
files are errors, with a filename, JSON path and expected value. There is no
compiled fallback. Installed worlds live in `/usr/lib/omadrop/worlds/`, beside
`bin/`. For a source build, set `OMADROP_WORLDS` to this directory:

```sh
OMADROP_WORLDS="$PWD/worlds" experiments/osaka-live/build/omadrop-osaka
```

Numbers use exact decimal doubles; existing float conversions happen at the
same drawing boundaries. Every profile field is required. Instance IDs are
unique across all stages and slots; stage and slot array order is significant.
Version 1 currently accepts the Osaka world and its named library profiles.

Art paths, shaders and procedural behavior remain in the kit. SVG validation
is available below; scene art bindings and label shorthand come in later rounds.
No general SVG renderer,
expressions, hot reload or executable world code is supported today.

## Supported SVG

SVG subset version 1 compiles artwork into the existing Canvas. It is available
as an import/check tool; Osaka does not load SVG artwork yet. Use a 1920 by 1080
artboard with `viewBox="0 0 1920 1080"`.

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
