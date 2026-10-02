#!/usr/bin/env python3
"""Onset-locked visible change. For each run, character updates per frame are
divided by their own 2 s local mean (so effect phases with many or few updates
weigh alike), then averaged in 50 ms bins around the REFERENCE log's kicks and
snares. A run that follows those hits rises right after 0 ms; a run driven by
other audio (shifted, different) or by the stock clock stays flat near 1.

usage: lockin.py REFERENCE.csv RUN.csv [RUN.csv ...]
"""
import csv
import sys


def load(p):
    with open(p) as f:
        return list(csv.DictReader(f))


ref = load(sys.argv[1])
sections = [(0, 36), (36, 66), (66, 96), (96, 120)]
for path in sys.argv[2:]:
    run = load(path)
    n = min(len(run), len(ref))
    ch = [int(r["changes"]) for r in run[:n]]
    tempo = [float(r["tempo"]) for r in run[:n]]
    pre = [0]
    for x in ch:
        pre.append(pre[-1] + x)
    w = 240
    rel = []
    for i in range(n):
        a, b = max(0, i - w // 2), min(n, i + w // 2)
        m = (pre[b] - pre[a]) / (b - a)
        rel.append(ch[i] / m if m > 0.5 else None)
    onsets = [i for i, r in enumerate(ref[:n]) if r["kick"] == "1" or r["snare"] == "1"]
    bins = []
    for start in range(-24, 48, 6):
        vals = [rel[i + k] for i in onsets for k in range(start, start + 6)
                if 0 <= i + k < n and rel[i + k] is not None]
        bins.append("%+4d %.2f" % (start * 1000 // 120, sum(vals) / len(vals) if vals else 0))
    sec = " ".join("%.2f" % (sum(tempo[a * 120:b * 120]) / max(1, len(tempo[a * 120:b * 120]))) for a, b in sections)
    print("%s  tempo by section %s" % (path, sec))
    print("   " + " | ".join(bins))
