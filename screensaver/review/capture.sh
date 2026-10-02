#!/bin/bash
# Runs inside the box: steps the review terminal through a recorded music run
# and captures the real terminal at 60 fps (every second 120 Hz display frame).
# usage: capture.sh FRAMES OUT.mp4 [SETTLE_SECONDS]
set -euo pipefail
frames=$1 out=$2 settle=${3:-0.045}
dir=$HOME/replay
exec 3>"$dir/control" 4<"$dir/ack"
shot=$dir/frame.ppm
size=$((1920 * 1080 * 3 + 17))
for ((k = 0; k < frames; k++)); do
  echo $((2 * k + 1)) >&3
  read -r _ <&4
  sleep "$settle"
  # a failed or short screenshot would drop a frame and shift the sync
  for try in 1 2 3 4 5; do
    grim -t ppm "$shot" 2>/dev/null && [[ $(stat -c %s "$shot") -eq $size ]] && break
    sleep 0.02
  done
  cat "$shot"
done | ffmpeg -loglevel error -y -f image2pipe -framerate 60 -c:v ppm -i - \
  -c:v libx264 -preset ultrafast -crf 14 -pix_fmt yuv420p "$out"
