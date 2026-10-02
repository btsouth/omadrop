#!/usr/bin/env python3
"""Per-effect musical response on a fixture: each stock effect is run alone on
the music clock and its rewritten cells per frame are averaged around the
fixture's kicks and snares (normalized by a 2 s local mean). Contrast is the
mean in the 0..150 ms after hits over the 150 ms before them.

usage: per_effect.py FIXTURE.f32 SECONDS [OFFSET]
"""
import csv
import os
import subprocess
import sys

here = os.path.dirname(os.path.abspath(__file__))
fixture, seconds = sys.argv[1], sys.argv[2]
offset = sys.argv[3] if len(sys.argv) > 3 else "0"
binary = os.environ.get("TTFX_MUSIC") or os.path.join(here, "..", "build", "ttfx-music")
names = subprocess.run([binary, "--help"], capture_output=True, text=True).stdout
effects = []
on = False
for line in names.splitlines():
    if line.startswith("Commands:"):
        on = True
        continue
    if on:
        if not line.startswith("  "):
            break
        name = line.split()[0]
        if name != "help":
            effects.append(name)
if os.environ.get("EFFECTS"):
    effects = [e for e in effects if e in os.environ["EFFECTS"].split(",")]
results = []
tempos = []
for name in effects:
    log = "/tmp/per-effect-%s.csv" % name
    subprocess.run([binary, "-i", os.path.expanduser("~/.config/omarchy/branding/screensaver.txt"),
                    "--frame-rate", "120", "--canvas-width", "137", "--canvas-height", "33",
                    "--ignore-terminal-dimensions", "--reuse-canvas", "--anchor-canvas", "c",
                    "--anchor-text", "c", "--no-eol", "--no-restore-cursor", "--seed", "3",
                    "--virtual-clock", "--music-seconds", seconds, "--music-log", log,
                    "--music", fixture, "--music-offset", offset, "--include-effects", name],
                   stdout=subprocess.DEVNULL, check=True)
    with open(log) as f:
        rows = list(csv.DictReader(f))
    os.remove(log)
    # first run of the effect only
    end = next((i for i in range(1, len(rows)) if rows[i]["bytes"] != rows[i - 1]["bytes"]
                and int(rows[i]["changes"]) == 0 and False), len(rows))
    ch = [int(r["changes"]) for r in rows]
    tempos.extend(float(r["tempo"]) for r in rows)
    n = len(ch)
    pre = [0]
    for x in ch:
        pre.append(pre[-1] + x)
    rel = []
    for i in range(n):
        a, b = max(0, i - 120), min(n, i + 120)
        m = (pre[b] - pre[a]) / (b - a)
        rel.append(ch[i] / m if m > 0.5 else None)
    onsets = [i for i, r in enumerate(rows) if r["kick"] == "1" or r["snare"] == "1"]
    after = [rel[i + k] for i in onsets for k in range(0, 18) if i + k < n and rel[i + k] is not None]
    before = [rel[i - k] for i in onsets for k in range(1, 19) if i - k >= 0 and rel[i - k] is not None]
    contrast = (sum(after) / len(after)) / (sum(before) / len(before)) if after and before else 0
    still = 100 * sum(1 for x in ch if x == 0) / n
    results.append((contrast, name, sum(ch) / n, still))
print("mean tempo %.2f, mean contrast %.2f" % (sum(tempos) / len(tempos), sum(r[0] for r in results) / len(results)))
for contrast, name, mean, still in sorted(results):
    print("%-16s contrast %.2f  cells/frame %7.1f  still frames %3.0f%%" % (name, contrast, mean, still))
