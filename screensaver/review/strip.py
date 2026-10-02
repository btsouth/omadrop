#!/usr/bin/env python3
"""Consecutive frames of a captured review video as one labeled sheet, for
judging motion around musical events by eye.

usage: strip.py VIDEO RUN.csv START END STEP_FRAMES OUT.png [COLUMNS] [SCALE] [X,Y,W,H]

Each tile is labeled with its time and with K/S/H when the run's log saw a
kick, snare or hat in the 1/60 s before that frame.
"""
import csv
import subprocess
import sys

import numpy as np
from PIL import Image, ImageDraw

video, log, start, end, step, out = sys.argv[1:7]
columns = int(sys.argv[7]) if len(sys.argv) > 7 else 6
scale = float(sys.argv[8]) if len(sys.argv) > 8 else 0.25
crop = [int(v) for v in sys.argv[9].split(",")] if len(sys.argv) > 9 else [0, 0, 1920, 1080]
start, end, step = float(start), float(end), int(step)
with open(log) as f:
    rows = list(csv.DictReader(f))
w, h = 1920, 1080
raw = subprocess.run(
    ["ffmpeg", "-loglevel", "error", "-ss", "%.3f" % start, "-t", "%.3f" % (end - start),
     "-i", video, "-f", "rawvideo", "-pix_fmt", "rgb24", "-"],
    check=True, capture_output=True).stdout
frames = np.frombuffer(raw, np.uint8).reshape(-1, h, w, 3)[::step]
cx, cy, w, h = crop
frames = frames[:, cy:cy + h, cx:cx + w]
tw, th = int(w * scale), int(h * scale)
rows_n = (len(frames) + columns - 1) // columns
sheet = Image.new("RGB", (columns * tw, rows_n * (th + 16)), (40, 40, 40))
draw = ImageDraw.Draw(sheet)
for k, frame in enumerate(frames):
    t = start + k * step / 60
    i = int(round(t * 120))
    marks = ""
    for r in rows[max(0, i - 2):i + 1]:
        marks += ("K" if r["kick"] == "1" else "") + ("S" if r["snare"] == "1" else "") + ("H" if r["hat"] == "1" else "")
    tempo = rows[min(i, len(rows) - 1)]["tempo"]
    x, y = (k % columns) * tw, (k // columns) * (th + 16)
    sheet.paste(Image.fromarray(frame).resize((tw, th), Image.BILINEAR), (x, y + 16))
    draw.text((x + 4, y + 2), "%.3fs x%s %s" % (t, tempo, marks), fill=(255, 220, 80) if marks else (200, 200, 200))
sheet.save(out)
print(out, len(frames), "frames")
