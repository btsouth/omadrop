# Third-party notices

## projectM

Omadrop uses projectM 4.1.7 at commit
`e0b0a967f0ffd7d332106c366668ed271718472b`, with its pinned projectm-eval
submodule `da885dcdf33620ef26aa04cac9e215378b80252e`. Omadrop adds eight bounded
per-instance audio variables to the evaluator. The library remains dynamically
linked and can be rebuilt with `bin/build-milkdrop-runtime`.

The complete modification recipe is in
`experiments/milkdrop-audio-pilot/prepare.py`; `bin/fetch-projectm` retrieves the
matching upstream source. projectM is LGPL-2.1-or-later; projectm-eval and hlslparser are MIT
licensed. Their license texts are under `third-party/projectm/`. These components are not relicensed as MIT
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
rights. The provenance record identifies the source of each file; it does not establish
an individual license grant from each artist. Omadrop does not relicense these
textures under MIT.

## Legacy compatibility presets

The source checkout retains classic presets and the Reactive Tunnel, Reactive
Orbit and Reactive Wire adaptations as compatibility and test inputs. Their
source is [the classic projectM collection](https://github.com/projectM-visualizer/presets-projectm-classic)
at `14a6244a7d32eb7e114e1a92d1cb93358cdcc54a`. Filenames preserve credits to
Aderrasi, Geiss, Martin, Tokyo, Unchained, Rovastar, fiShbRaiN, Krash and their
collaborators. They retain their authors' rights and are not MIT licensed by
Omadrop. The three Omadrop adaptations retain the underlying preset credits.

## Terminal text effects (ttfx)

Omarchy mode uses Omadrop's music-driven fork of [ttfx](https://github.com/omacom/ttfx),
based on v0.5.0, commit `112ebb310b848d8f1251a5c2919a9cd5a5c78e18`.
Copyright (c) 2026 37signals / omacom-io. ttfx ports the effects and engine of
[TerminalTextEffects](https://github.com/ChrisBuilds/terminaltexteffects),
copyright (c) 2023 ChrisBuilds. Both are MIT licensed. The full license and
attribution notice are in `screensaver/ttfx/LICENSE` and `screensaver/ttfx/NOTICE`,
installed as `licenses/ttfx-LICENSE` and `licenses/ttfx-NOTICE`. Fork changes are
described in `screensaver/ttfx/OMADROP-PROVENANCE.md`.

## Qt and system libraries

The controls dynamically link to system Qt 6 libraries from Arch's `qt6-base`
and `qt6-declarative` packages. Qt is copyright The Qt Company and contributors;
these modules are available under LGPL-3.0 or commercial terms. Omadrop does
not bundle or modify Qt. The system packages supply their own license notices.
See [Qt licensing](https://doc.qt.io/qt-6/licensing.html). Other dynamically
linked system libraries retain their respective package licenses.

## Fonts and media

No font files are shipped. The controls and website use installed system fonts.
Naming a font in the CSS does not bundle it. The Omarchy logo is read from the
user's Omarchy installation, rather than copied into the package.

Scene screenshots and the website's `collection.mp4` show credited MilkDrop
presets and textures; these images retain the underlying artists' rights.
Effect thumbnails show ttfx / TerminalTextEffects output and keep those credits.
The Omadrop icon and site graphics are covered by Omadrop's MIT license.

The test fixture `tests/fixtures/collection-music.ogg` and the website video use
"beat me" by smilingcynic (Christopher Hawes), from
[ccMixter](https://ccmixter.org/files/smilingcynic/41102), under
[CC BY 3.0 Unported](https://creativecommons.org/licenses/by/3.0/).
The fixture is a 24-second excerpt transcoded to Ogg Vorbis; the website video
excerpts, transcodes and synchronizes the music with visuals. Full credit and
change notices are in `tests/fixtures/collection-music-attribution.txt` and
`site/public/media/collection-attribution.txt`. Audio is not installed in the
runtime package. `demo/music-rights.json` records approved recording sources;
it does not bundle those full recordings.

The v0.5.0 launch video uses “We Can Fix Everything” by Kevin Koontz
([@koozeex1](https://x.com/koozeex1)), also credited on
[Omarchy’s website](https://omarchy.org/). The video uses a continuous
60-second excerpt with a short ending fade. Scene and preset-author credits
appear in the video. The song is not installed in the runtime package.

## Removal requests

If you own a preset, texture or other asset, request removal or corrected credit
through [GitHub issues](https://github.com/btsouth/omadrop/issues/new/choose).
Identify the work and your connection to it, and say what you would like changed.
