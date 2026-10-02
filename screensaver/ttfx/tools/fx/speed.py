#!/usr/bin/env python3
"""Original Rust engine vs fx wall time per effect, and their geometric-mean speedup.

Both engines run from the same binary (TTFX_FX=0 / TTFX_FX=force) on a
200x50 canvas with the oracle's 190x46 "big" text, --frame-rate 0, seed 1,
output to /dev/null, pinned to one core, best of N runs.

--cpu measures the child's user+system CPU time instead of wall time. It is
far less sensitive to other load on the machine (it still sees cache and SMT
contention), so use it when others are running; quote wall time from a quiet
machine as the headline.

--python adds the original Python TerminalTextEffects on the same workload
(its own RNG, since its random module is C and a parity shim would be
unfair). Point it at an interpreter that has the package, or install one:
    python3 -m venv target/tte-venv
    target/tte-venv/bin/pip install terminaltexteffects==0.15.0
and --python with no value uses that venv. matrix and thunderstorm are
clock-bound in Python (no virtual clock there), so they get no Python ratio.

Usage: speed.py [--bin PATH] [--core N] [--runs N] [--fx-only] [--cpu]
                [--python [PY]] [--python-runs N] [effect ...]
"""
import argparse
import math
import os
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CLOCKED = {"matrix", "thunderstorm"}  # read the clock: time them on the virtual one

p = argparse.ArgumentParser()
p.add_argument("--bin", default=os.path.join(ROOT, "target/release/ttfx"))
p.add_argument("--core", default=os.environ.get("SPEED_CORE", "8"))
p.add_argument("--runs", type=int, default=5)
p.add_argument("--fx-only", action="store_true", help="skip the original engine's runs")
p.add_argument("--cpu", action="store_true", help="child CPU time instead of wall time")
p.add_argument("--python", nargs="?", const=os.path.join(ROOT, "target/tte-venv/bin/python"),
               help="also time Python TTE with this interpreter")
p.add_argument("--python-runs", type=int, default=1, help="Python runs per effect (it is slow)")
p.add_argument("effects", nargs="*")
a = p.parse_args()

effects = a.effects or sorted(
    f[:-3] for f in os.listdir(os.path.join(ROOT, "src/fx/effects"))
    if f.endswith(".rs") and f != "mod.rs")

text = os.path.join(ROOT, "target/speed-input.txt")
line = ("The quick brown fox jumps over the lazy dog 0123456789 " * 4)[:190]
os.makedirs(os.path.dirname(text), exist_ok=True)
with open(text, "w") as f:
    f.write("\n".join(line for _ in range(46)))


WORKLOAD = ["--frame-rate", "0", "--canvas-width", "200", "--canvas-height", "50", "--ignore-terminal-dimensions"]
if a.python and not os.path.exists(a.python):
    sys.exit(f"{a.python} not found; see speed.py --help for installing Python TTE")


def best(engine, effect):
    runs = a.runs
    if engine == "python":
        args = ["taskset", "-c", a.core, a.python, "-m", "terminaltexteffects"] + WORKLOAD + [effect]
        runs = a.python_runs
    else:
        args = ["taskset", "-c", a.core, a.bin]
        if effect in CLOCKED:
            args.append("--virtual-clock")
        args += ["--seed", "1"] + WORKLOAD + [effect]
    env = dict(os.environ, TTFX_FX=engine)
    fastest = math.inf
    for _ in range(runs):
        with open(text) as stdin:
            start = time.perf_counter()
            child = subprocess.Popen(args, stdin=stdin, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, env=env)
            stderr = child.stderr.read()
            _, status, usage = os.wait4(child.pid, 0)
            elapsed = time.perf_counter() - start
        if a.cpu:
            elapsed = usage.ru_utime + usage.ru_stime
        code = os.waitstatus_to_exitcode(status)
        if code != 0:
            sys.exit(f"{effect} TTFX_FX={engine} exited {code}: {stderr.decode()[:200]}")
        fastest = min(fastest, elapsed)
    return fastest * 1000


logs, py_logs, py_rust_logs = [], [], []
if a.python:
    print(f"{'effect':16} {'python ms':>10} {'rust ms':>8} {'fx ms':>8} {'py/fx':>7} {'rust/fx':>8}")
else:
    print(f"{'effect':16} {'rust ms':>8} {'fx ms':>8} {'x':>6}")
for effect in effects:
    fx = best("force", effect)
    if a.fx_only:
        print(f"{effect:16} {'':>8} {fx:8.1f}", flush=True)
        continue
    rust = best("0", effect)
    logs.append(math.log(rust / fx))
    if a.python:
        if effect in CLOCKED:
            print(f"{effect:16} {'n/a':>10} {rust:8.1f} {fx:8.1f} {'n/a':>7} {rust / fx:8.2f}", flush=True)
            continue
        py = best("python", effect)
        py_logs.append(math.log(py / fx))
        py_rust_logs.append(math.log(py / rust))
        print(f"{effect:16} {py:10.1f} {rust:8.1f} {fx:8.1f} {py / fx:7.1f} {rust / fx:8.2f}", flush=True)
    else:
        print(f"{effect:16} {rust:8.1f} {fx:8.1f} {rust / fx:6.2f}", flush=True)
if logs:
    print(f"geomean speedup {math.exp(sum(logs) / len(logs)):.2f}x over {len(logs)} effects")
if py_logs:
    print(f"geomean vs python: fx {math.exp(sum(py_logs) / len(py_logs)):.1f}x, "
          f"rust {math.exp(sum(py_rust_logs) / len(py_rust_logs)):.1f}x over {len(py_logs)} effects")
