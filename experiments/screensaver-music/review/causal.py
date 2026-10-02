#!/usr/bin/env python3
"""Does visible change follow the music? For each run, the per-frame count of
character updates is compared with the onsets of a reference audio log.

usage: causal.py REFERENCE.csv RUN.csv [RUN.csv ...]

Reports, per run: mean updates per frame in each section; the onset-locked
response (updates in the 250 ms after reference kicks/snares, relative to the
250 ms before them); and the correlation of update rate with the reference's
onset envelope.
"""
import csv
import math
import sys


def load(path):
    with open(path) as f:
        return list(csv.DictReader(f))


ref = load(sys.argv[1])
sections = [(0, 36, "reference"), (36, 66, "beat me"), (66, 96, "sparse piano"), (96, 120, "reference")]
onsets = [i for i, r in enumerate(ref) if r["kick"] == "1" or r["snare"] == "1"]
# onset envelope: decaying impulses at reference onsets
env = [0.0] * len(ref)
level = 0.0
on = set(onsets)
for i in range(len(ref)):
    level *= math.exp(-1 / (120 * 0.12))
    if i in on:
        level += 1.0
    env[i] = level


def corr(a, b):
    n = min(len(a), len(b))
    a, b = a[:n], b[:n]
    ma, mb = sum(a) / n, sum(b) / n
    cov = sum((x - ma) * (y - mb) for x, y in zip(a, b))
    va = sum((x - ma) ** 2 for x in a)
    vb = sum((y - mb) ** 2 for y in b)
    return cov / math.sqrt(va * vb) if va > 0 and vb > 0 else 0.0


for path in sys.argv[2:]:
    run = load(path)
    ch = [int(r["changes"]) for r in run]
    n = min(len(ch), len(ref))
    parts = []
    for a, b, label in sections:
        sel = ch[int(a * 120):min(int(b * 120), n)]
        if sel:
            parts.append("%s %.1f" % (label, sum(sel) / len(sel)))
    after = before = 0
    count = 0
    for i in onsets:
        if 30 <= i < n - 30:
            after += sum(ch[i:i + 30])
            before += sum(ch[i - 30:i])
            count += 1
    lock = after / before if before else float("inf")
    # smooth update rate over 100 ms before correlating
    sm = []
    acc = 0.0
    for x in ch[:n]:
        acc = acc * math.exp(-1 / 12) + x
        sm.append(acc)
    effects = []
    last = None
    for r in run:
        if r["effect"] != last:
            effects.append("%s@%.0f" % (r["effect"], float(r["time"])))
            last = r["effect"]
    print(path)
    print("  updates/frame: " + ", ".join(parts))
    print("  onset-locked response %.2fx over %d onsets; correlation with onset envelope %.2f" % (lock, count, corr(sm, env[:n])))
    print("  effects: " + " ".join(effects))
