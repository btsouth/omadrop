#!/usr/bin/env python3
"""Compose W1 frames; requires Pillow. Music and machine evidence stay external."""
import argparse
import csv
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont


def strip(root, mode, columns, width):
    folder = root / mode
    frames = sorted(folder.glob(f"{mode}-[0-9][0-9].png"))
    expected = 12 if mode == "sweep" else 30
    if len(frames) != expected:
        raise SystemExit(f"{mode}: expected {expected} frames, found {len(frames)}")
    with (folder / f"{mode}-parameters.csv").open() as stream:
        rows = list(csv.DictReader(stream))
    if len(rows) != expected:
        raise SystemExit(f"{mode}: mismatched parameter series")
    height = width * 9 // 16
    result = Image.new("RGB", (columns * width, ((expected + columns - 1) // columns) * (height + 30)), "#e8daba")
    draw = ImageDraw.Draw(result)
    try:
        font = ImageFont.truetype("DejaVuSans.ttf", 16)
    except OSError:
        try:
            font = ImageFont.load_default(size=16)
        except TypeError:
            font = ImageFont.load_default()
    for i, (path, row) in enumerate(zip(frames, rows)):
        x, y = (i % columns) * width, (i // columns) * (height + 30)
        with Image.open(path) as source:
            result.paste(source.resize((width, height), Image.Resampling.LANCZOS), (x, y + 30))
        label = f"Q {float(row['Q']):.3f} | A 330 px" if mode == "sweep" else f"{float(row['seconds']):.0f} s | constant music controls"
        draw.text((x + 8, y + 5), label, font=font, fill="#102955")
    result.save(root / ("steepness-sweep-strip.png" if mode == "sweep" else "group-travel-strip.png"))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    strip(args.directory, "sweep", 4, 640)
    strip(args.directory, "travel", 5, 384)
