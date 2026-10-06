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
same drawing boundaries. Every profile field is required. Instance IDs are
unique across stages, slots and explicit nodes; stage and slot array order is
significant. Version 1 accepts the Osaka world, the window shorthand example
and their named library profiles.

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

## Make a window react to music

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
   If the world labels layers `window.band0` to `window.band5`, each of those
   layers must visibly respond too, and the ones that never do are listed.
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
