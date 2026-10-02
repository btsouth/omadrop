# Third-party notices

## projectM

Omadrop uses projectM 4.1.7 at commit
`e0b0a967f0ffd7d332106c366668ed271718472b`, with its pinned projectm-eval
submodule `da885dcdf33620ef26aa04cac9e215378b80252e`. Omadrop adds eight bounded
per-instance audio variables to the evaluator. The library remains dynamically
linked and can be rebuilt with `bin/build-milkdrop-runtime`.

The complete modification recipe is in
`experiments/milkdrop-audio-pilot/prepare.py`; `bin/fetch-projectm` retrieves the
matching upstream source. Licenses for projectM, projectm-eval and hlslparser
are under `third-party/projectm/`. These components are not relicensed as MIT
by Omadrop.

## Curated MilkDrop collection

The original presets came from [Ryan Geiss's January 2021 favorites](https://www.geisswerks.com/milkdrop/favorite_presets_2021_01_03.zip).
Original archive and preset hashes are recorded in
`presets/milkdrop-originals/manifest.json`. Omadrop's display names and audio
adaptations are described below; original credited filenames are preserved.

| Display name | Original credited preset |
| --- | --- |
| Cloud Cubes | flexi - bouncing icecubes.milk |
| Fractal Caves | martin - mandelbox explorer - wreck diver nz+ phranq.milk |
| Ice Wave | martin - rogue wave -ps3.milk |
| Color Spiral | yin - 350 - Chromatron.milk |
| Living Cells | _Geiss - Reaction Diffusion 3 (Lichen Relief Mix).milk |
| Liquid Ice | Flexi - crush ice 62.milk |
| Neon Orbits | Serge + martin - crystal palace010.milk |
| Aqua Filaments | Goody + martin - crystal palace - Aqua Lumens5.milk |
| Pixie Swarm | martin - pixies party.milk |
| Desert Rose | Geiss - Desert Rose 4.milk |
| Silk Spiral | flexi - swing out on the spiral.milk |
| Crystal Palace | martin - crystal palace.milk |
| Organic Light | martin - organic light.milk |
| Peacock Weave | TonyMilkdrop - Dawning The Peacocks [Flexi - quirks + multiverse].milk |
| Infinity Layers | martin - infinity.milk |
| Spiral of Light | LuX - Spiral of Light (Minimal Mix).milk |
| Fractal Flight | martin - mandelbox explorer - high speed demo version.milk |
| Water Glowsticks | Eo.S. + Geiss - glowsticks v2 03 music shifter edit b (water mix).milk |
| Silver Pixies | Martin - Pixies Party (Hakan mash-up) 9-1.milk |
| Magic Carpet | Hexcollie - Amorphous Magic carpet.milk |
| Firesticks | Tripgnosis - Firesticks.milk |

Each adaptation adds local frequency or timbral response to existing authored
behavior. Original artwork, shaders and preset authorship remain with their
creators. Adaptations are not presented as wholly original Omadrop scenes.

MilkDrop community collections often lack individual license terms. The
[projectM preset notice](https://github.com/projectM-visualizer/presets-cream-of-the-crop/blob/master/LICENSE.md)
describes that history and a removal practice. Omadrop preserves attribution and
accepts correction or removal requests through its GitHub issues. This notice
is not a claim that all presets are public domain or MIT licensed.

## Textures

Textures come from the [projectM MilkDrop texture pack](https://github.com/projectM-visualizer/presets-milkdrop-texture-pack)
at `b6e461010ffbb78939d5b3c2248bb98af77411c3`. Exact file hashes are recorded in
`presets/textures/PROVENANCE.json`. Textures retain their original authorship and
rights. Public distribution terms for the selected artwork remain a release
review item; this candidate does not establish permission from each author.

## Legacy compatibility presets

The older classic collection and its Reactive Tunnel, Reactive Orbit and
Reactive Wire adaptations retain the notices in
[legacy preset credits](docs/legacy-preset-notices.md).

## Terminal text effects (ttfx)

The music screensaver is driven by ttfx-music, a Rust port of
[terminaltexteffects](https://github.com/ChrisBuilds/terminaltexteffects) by
ChrisBuilds. ttfx is MIT licensed; its LICENSE and NOTICE ship alongside this
notice as `screensaver/ttfx/LICENSE` and `screensaver/ttfx/NOTICE`.
