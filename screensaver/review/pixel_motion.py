#!/usr/bin/env python3
"""Visible change measured on the captured terminal itself: the fraction of
pixels that changed between consecutive 60 fps frames, written per frame and
summarized against the run's onsets like lockin.py.

usage: pixel_motion.py VIDEO REFERENCE.csv OUT.csv
"""
import csv
import subprocess
import sys

import numpy as np

video, ref_path, out = sys.argv[1:4]
w, h = 480, 270
proc = subprocess.Popen(
    ["ffmpeg", "-loglevel", "error", "-i", video, "-vf", "scale=%d:%d:flags=area" % (w, h),
     "-f", "rawvideo", "-pix_fmt", "gray", "-"], stdout=subprocess.PIPE)
prev = None
motion = []
while True:
    buf = proc.stdout.read(w * h)
    if len(buf) < w * h:
        break
    cur = np.frombuffer(buf, np.uint8).astype(np.int16)
    motion.append(0.0 if prev is None else float((np.abs(cur - prev) > 12).mean()))
    prev = cur
with open(out, "w") as f:
    f.write("frame,time,changed\n")
    for k, m in enumerate(motion):
        f.write("%d,%.4f,%.6f\n" % (k, k / 60, m))
with open(ref_path) as f:
    ref = list(csv.DictReader(f))
n = len(motion)
pre = [0.0]
for m in motion:
    pre.append(pre[-1] + m)
rel = []
for i in range(n):
    a, b = max(0, i - 60), min(n, i + 60)
    mean = (pre[b] - pre[a]) / (b - a)
    rel.append(motion[i] / mean if mean > 1e-4 else None)
# reference onsets (120 Hz log) as 60 fps video frames; video frame k shows
# display frame 2k+1
onsets = sorted({(i - 1) // 2 for i, r in enumerate(ref) if r["kick"] == "1" or r["snare"] == "1"})
bins = []
for s in range(-12, 24, 3):
    vals = [rel[i + k] for i in onsets for k in range(s, s + 3) if 0 <= i + k < n and rel[i + k] is not None]
    bins.append("%+4d %.2f" % (s * 1000 // 60, sum(vals) / len(vals) if vals else 0))
print(video)
print("   " + " | ".join(bins))
