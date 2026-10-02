#!/usr/bin/env python3
"""Runs inside the review terminal: writes a recorded music run's exact
bytes up to the display frame the capture driver asks for.

usage: replay.py RUN.ans RUN.csv CONTROL_FIFO ACK_FIFO
"""
import csv
import os
import sys

ans_path, csv_path, control_path, ack_path = sys.argv[1:5]
with open(csv_path) as f:
    offsets = [int(row["bytes"]) for row in csv.DictReader(f)]
data = open(ans_path, "rb").read()
control = open(control_path)
ack = open(ack_path, "w")
# omarchy-screensaver sets a black background before starting ttfx
os.write(1, b"\033]11;rgb:00/00/00\007")
written = 0
view = memoryview(data)
for line in control:
    frame = int(line)
    target = offsets[min(frame, len(offsets) - 1)]
    while written < target:
        written += os.write(1, view[written:target])
    ack.write("ok\n")
    ack.flush()
