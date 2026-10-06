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

Art paths, shaders and procedural behavior remain in the kit. SVG art import
and label shorthand expansion come in later rounds. No general SVG renderer,
expressions, hot reload or executable world code is supported today.
