#!/bin/bash
# Offline review run at the real screensaver's settings and grid (Ghostty,
# font size 18, 1920x1080 = 137x33): writes NAME.ans (exact terminal bytes)
# and NAME.csv (one line per 120 Hz display frame).
# usage: render-run.sh NAME SEED SECONDS MUSIC-ARGS...
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
name=$1 seed=$2 seconds=$3
shift 3
"$here/../build/ttfx-music" -i "${SCREENSAVER_TEXT:-$HOME/.config/omarchy/branding/screensaver.txt}" \
  --frame-rate 120 --canvas-width 137 --canvas-height 33 --ignore-terminal-dimensions \
  --reuse-canvas --anchor-canvas c --anchor-text c --no-eol --no-restore-cursor \
  --seed "$seed" --virtual-clock --music-seconds "$seconds" --music-log "$name.csv" "$@" >"$name.ans"
