"""Render one representative frame per ttfx effect into a JPEG thumbnail.

Builds on the frame reader and ANSI/block renderer in
``screensaver/ttfx/tools/demo/make_gif.py``: it dumps the same parity frames at
a 96x27 canvas, scores the frames in the middle of the animation, and renders
the busiest one with Pillow to ``app/assets/effects/<slug>.jpg`` at 480x270.

Usage: make-effect-thumbnails.py [effect ...]
"""

from __future__ import annotations

import os
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
DEMO_DIR = REPO / "screensaver/ttfx/tools/demo"
OUT_DIR = REPO / "app/assets/effects"

# The screensaver animates the Omarchy logo, so the thumbnails do too.
LOGO = Path(os.environ.get("OMADROP_THUMBNAIL_TEXT", "/usr/share/omarchy/logo.txt"))
_lines = LOGO.read_text().rstrip("\n").splitlines()
ROWS = len(_lines) + 6
COLS = max(max(len(line) for line in _lines) + 8, ROWS * 32 // 9)
WIDTH = 480
HEIGHT = 270
CELL_W = WIDTH / COLS
CELL_H = HEIGHT / ROWS
FONT_SIZE = max(4, int(CELL_H * 0.85))
BG = (0, 0, 0)  # the screensaver runs on black
QUALITY = 80

EXTRA_ARGS: dict[str, list[str]] = {}

# make_gif.py reads these at import time to size its parity dump.
os.environ.setdefault("TTFX_DEMO_COLS", str(COLS))
os.environ.setdefault("TTFX_DEMO_ROWS", str(ROWS))
os.environ.setdefault("TTFX_DEMO_TEXT_FILE", str(LOGO))
sys.path.insert(0, str(DEMO_DIR))

import make_gif  # noqa: E402
from PIL import Image, ImageDraw, ImageFont  # noqa: E402

if os.environ.get("OMADROP_TTFX"):
    make_gif.BIN = Path(os.environ["OMADROP_TTFX"])


def effect_names() -> list[str]:
    """The real effect subcommands, straight from ``ttfx --help``."""
    import subprocess

    result = subprocess.run([str(make_gif.BIN), "--help"], capture_output=True, text=True)
    names = []
    in_commands = False
    for line in result.stdout.splitlines():
        if line.startswith("Commands:"):
            in_commands = True
            continue
        if not in_commands:
            continue
        if not line.startswith("  "):
            break
        slug = line.strip().split(None, 1)[0]
        if slug and slug != "help":
            names.append(slug)
    return names


def nonblank(rows) -> int:
    count = 0
    for cells in rows:
        for ch, _, bg, _ in cells:
            if (ch and ch != " ") or bg is not None:
                count += 1
    return count


def representative_frame(frames: list[str]) -> str:
    """The busiest frame in the middle 30-70% of the effect."""
    n = len(frames)
    if n == 0:
        raise ValueError("no frames")
    lo, hi = int(n * 0.30), int(n * 0.70)
    if hi <= lo:
        lo, hi = 0, n
    best, best_score = frames[lo], -1
    for frame in frames[lo:hi]:
        score = nonblank(make_gif.parse_frame(frame))
        if score > best_score:
            best, best_score = frame, score
    if best_score <= 0:
        # A frame with nothing drawn is useless; fall back to the very end.
        best = frames[-1]
    return best


def load_font() -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    for candidate in (
        "/usr/share/fonts/TTF/JetBrainsMonoNerdFont-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/dejavu/DejaVuSansMono.ttf",
    ):
        if os.path.exists(candidate):
            return ImageFont.truetype(candidate, FONT_SIZE)
    return ImageFont.load_default()


def to_rgb(color):
    if isinstance(color, tuple):
        return color
    if isinstance(color, str) and color.startswith("#") and len(color) == 7:
        return tuple(int(color[i : i + 2], 16) for i in (1, 3, 5))
    return BG


def blend(fg, under, alpha: float):
    fg, under = to_rgb(fg), to_rgb(under)
    return tuple(int(round(fg[i] * alpha + under[i] * (1 - alpha))) for i in range(3))


def render(frame: str, font) -> Image.Image:
    rows = make_gif.parse_frame(frame)
    image = Image.new("RGB", (WIDTH, HEIGHT), BG)
    draw = ImageDraw.Draw(image)

    for r, cells in enumerate(rows[:ROWS]):
        for c, (ch, fg, bg, _) in enumerate(cells[:COLS]):
            x, y = c * CELL_W, r * CELL_H
            if bg:
                draw.rectangle([x, y, x + CELL_W, y + CELL_H], fill=bg)
            if ch in make_gif.BLOCKS:
                yf, hf, op = make_gif.BLOCKS[ch]
                under = bg if bg else BG
                fill = blend(fg, under, op)
                draw.rectangle([x, y + yf * CELL_H, x + CELL_W, y + (yf + hf) * CELL_H], fill=fill)
            elif ch in make_gif.HALF_BLOCKS:
                xf, wf = make_gif.HALF_BLOCKS[ch]
                draw.rectangle(
                    [x + xf * CELL_W, y, x + (xf + wf) * CELL_W, y + CELL_H], fill=fg
                )
            elif ch and ch != " ":
                draw.text((x, y), ch, fill=fg, font=font)
    return image


def main(argv: list[str]) -> int:
    if not make_gif.BIN.exists():
        print(f"missing ttfx binary: {make_gif.BIN}", file=sys.stderr)
        print("build it with: cargo build --release --locked", file=sys.stderr)
        return 1
    effects = argv[1:] or effect_names()
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    font = load_font()
    failures = []
    for slug in effects:
        try:
            frames = make_gif.read_frames([slug] + EXTRA_ARGS.get(slug, []))
            frame = representative_frame(frames)
            image = render(frame, font)
            image.save(OUT_DIR / f"{slug}.jpg", "JPEG", quality=QUALITY)
            print(f"{slug}: {len(frames)} frames")
        except Exception as exc:  # noqa: BLE001 - report and keep going
            failures.append(slug)
            print(f"{slug}: FAILED: {exc}", file=sys.stderr)
    if failures:
        print(f"{len(failures)} effect(s) failed: {', '.join(failures)}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
