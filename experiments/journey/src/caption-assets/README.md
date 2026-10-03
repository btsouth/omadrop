# Caption outlines

`outlines.inc` contains only the fixed artwork for `OMARCHY THEME`,
`Osaka Jade` and `Kanagawa`. The caption renderer reads these absolute
M/L/Q/C/Z paths into cached `QPainterPath` objects. It needs no font files,
font lookup, QtSvg, display, or `QGuiApplication`. Native playback and JPEG
export use the same paths and painting code.

The outlines derive from Noto Sans Medium, version 2.015, with this embedded
copyright notice:

Copyright 2022 The Noto Project Authors
(https://github.com/notofonts/latin-greek-cyrillic)

The font's embedded license record identifies the SIL Open Font License 1.1;
the full license and copyright are included in `OFL.txt`. No complete font
is bundled. The derivative artwork converts the three strings to unhinted
vector curves, scales them to 11 px (label) and 46 px (titles), applies 2 px
tracking to the label, aligns the left edges, flips Y for painter coordinates,
and rounds coordinates to 0.001 px. Preserve the license and attribution when
redistributing these assets.

## Reproduce

Use Python 3 and an existing FreeType shared library. The generator uses only
Python's standard library via `ctypes`; it neither installs dependencies nor
loads Qt. Supply the same Noto Sans Medium TTF used in the generated file's
SHA-256 comment. The original input on this machine was
`/usr/share/fonts/noto/NotoSans-Medium.ttf`.

```sh
python experiments/journey/src/caption-assets/generate.py --font /path/NotoSans-Medium.ttf
python experiments/journey/src/caption-assets/generate.py --font /path/NotoSans-Medium.ttf --check
```

The generator writes beside itself by default, or accepts `--output`.
It prints path byte counts, command counts and control bounds. It uses
unscaled FreeType outlines and available legacy kerning, without a shaping
engine; these fixed Latin captions do not require complex-script shaping.
Different font revisions can change the generated paths and source digest.

## Integration

Include `caption.h` in the scene renderer and call
`Journey::paintThemeCaption(painter, seconds)` after the art, before restoring
the normalized 1600 x 900 painter transform. The helper saves/restores its own
painter state. Osaka Jade appears during `[0, 5)` and Kanagawa during `[35, 40)`
movie seconds; both are transparent at their starting instant. Opacity follows
a smooth 0.6-second entrance and 1-second exit. The entrance also settles upward
by 8 normalized pixels. Captions are independent of audio and use a small,
feathered local shadow for legibility.

## Journey titles

`title-outlines.inc` holds the Journey titles (`OMARCHY THEME`, `Osaka Jade`,
`Kanagawa`) in JetBrains Mono, the Omarchy mono face: Bold 34 px names and
Regular 12.5 px label at 1920 x 1080. JetBrains Mono is licensed under the SIL
Open Font License 1.1. Regenerate with `python title_generate.py` (defaults to
the Nerd Font build Omarchy installs). The older `outlines.inc` is no longer
used by the renderer.

## Osaka Jade sign lettering

`sign-outlines.inc` holds seven glyphs (居酒屋 for the izakaya neon, らーめん
for the noodle cart's curtains), each at 1 px per em. They derive from Noto
Sans CJK JP Bold (SIL Open Font License 1.1, Copyright 2014-2021 Adobe, with
Reserved Font Name 'Source'; see `OFL.txt` for the license text). Regenerate
with `python3 sign_generate.py --font /usr/share/fonts/noto-cjk/NotoSansCJK-Bold.ttc`;
the font's SHA-256 is recorded in the generated file.
