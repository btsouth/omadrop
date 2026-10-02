"""Render the ASCII-filter version of each MilkDrop scene thumbnail.

Applies the renderer's braille-dot material (asciiEnabled branch of
experiments/projectm-ascii/live_compositor_shaders.h) with silent audio: 12x24
cells of 2x4 dots on a 6 px pitch, ordered thresholds, each dot colored from the scene under it. The dot pitch is kept at the shader's 6 px on a 480x270
image, so the dots stay readable at thumbnail size.

Usage: make-ascii-thumbnails.py
"""

from __future__ import annotations

from pathlib import Path

import numpy as np
from PIL import Image

REPO = Path(__file__).resolve().parents[2]
SOURCE_DIR = REPO / "site/public/media/scenes"
OUT_DIR = REPO / "app/assets/scenes-ascii"

WIDTH, HEIGHT = 480, 270
PITCH = 6
RADIUS = 1.90
EDGE = 0.48
SUPERSAMPLE = 3
# threshold[dy * 2 + dx] from the shader.
THRESHOLD = np.array([[0.08, 0.58], [0.33, 0.83], [0.70, 0.20], [0.95, 0.45]])


def render(source: Image.Image) -> Image.Image:
    pixels = np.asarray(source.convert("RGB").resize((WIDTH, HEIGHT), Image.LANCZOS),
                        dtype=np.float64) / 255.0
    columns, rows = WIDTH // PITCH, HEIGHT // PITCH
    blocks = pixels[: rows * PITCH, : columns * PITCH].reshape(rows, PITCH, columns, PITCH, 3)
    blocks = blocks.transpose(0, 2, 1, 3, 4).reshape(rows, columns, PITCH * PITCH, 3)
    light = blocks @ np.array([0.2126, 0.7152, 0.0722])
    pick = light.argmax(axis=2)
    best = np.take_along_axis(light, pick[..., None], axis=2)[..., 0]
    # At thumbnail scale one dot covers far more of the scene than on a real
    # display, so the dot takes the block's average hue at the brightest level.
    mean = blocks.mean(axis=2)
    mean_light = np.maximum(light.mean(axis=2), 1e-3)
    brightest = np.clip(mean * (best / mean_light)[..., None], 0.0, 1.0)

    level = np.minimum(1.0, np.sqrt(best) * 1.25)
    level = np.floor(level * 5.0 + 0.5) / 5.0
    threshold = THRESHOLD[np.arange(rows)[:, None] % 4, np.arange(columns)[None, :] % 2]
    lit = level >= threshold
    color = brightest * (0.80 + level * 0.32)[..., None]

    scale = SUPERSAMPLE
    size = PITCH * scale
    axis = (np.arange(size) + 0.5) / scale - PITCH / 2
    distance = np.hypot(axis[:, None], axis[None, :])
    t = np.clip((distance - (RADIUS - EDGE)) / (2 * EDGE), 0.0, 1.0)
    mask = 1.0 - t * t * (3.0 - 2.0 * t)

    dots = (color * lit[..., None])[:, None, :, None, :] * mask[None, :, None, :, None]
    canvas = np.zeros((HEIGHT * scale, WIDTH * scale, 3))
    canvas[: rows * size, : columns * size] = dots.reshape(rows * size, columns * size, 3)
    image = Image.fromarray((np.clip(canvas, 0.0, 1.0) * 255).astype(np.uint8))
    return image.resize((WIDTH, HEIGHT), Image.LANCZOS)


def main() -> int:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for source in sorted(SOURCE_DIR.glob("collection-*.jpg")):
        render(Image.open(source)).save(OUT_DIR / f"{source.stem}.png", optimize=True)
        print(source.stem)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
