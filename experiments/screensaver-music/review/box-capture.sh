#!/bin/bash
# Host side: replay NAME.ans/NAME.csv from the review directory through the
# real screensaver terminal (Ghostty, Omarchy's screensaver config, font size
# 18) in an omabox box and capture it at 60 fps, then mux the fixture audio.
# usage: box-capture.sh BOX REVIEW_DIR NAME SECONDS AUDIO.f32 [AUDIO_OFFSET]
set -euo pipefail
box=$1 dir=$2 name=$3 seconds=$4 audio=$5 offset=${6:-0}
here=$(cd "$(dirname "$0")" && pwd)
home=$(omabox -b "$box" path)/home
omabox -b "$box" run -- pkill -x ghostty || true
mkdir -p "$home/replay"
cp "$dir/$name.ans" "$dir/$name.csv" "$home/replay/"
rm -f "$home/replay/control" "$home/replay/ack" "$home/replay/$name.mp4"
mkfifo "$home/replay/control" "$home/replay/ack"
omabox -b "$box" hyprctl eval 'hl.config({ cursor = { invisible = true } })' >/dev/null || true
omabox -b "$box" run -d -q -- ghostty --class=org.omarchy.screensaver \
  --config-file=/usr/share/omarchy/default/ghostty/screensaver \
  --font-family="JetBrainsMono Nerd Font" --font-size=18 \
  -e python3 "$here/replay.py" "/home/sbx/replay/$name.ans" "/home/sbx/replay/$name.csv" \
  /home/sbx/replay/control /home/sbx/replay/ack
omabox -b "$box" wait window org.omarchy.screensaver >/dev/null
omabox -b "$box" windows | grep -q 'on-screen' ||
  omabox -b "$box" hyprctl dispatch 'hl.dsp.workspace.toggle_special("screensaver")' >/dev/null
omabox -b "$box" run -- "$here/capture.sh" $((seconds * 60)) "/home/sbx/replay/$name.mp4"
ffmpeg -loglevel error -y -i "$home/replay/$name.mp4" \
  -f f32le -ar 44100 -ac 2 -ss "$offset" -t "$seconds" -i "$audio" \
  -map 0:v -map 1:a -c:v copy \
  -c:a aac -b:a 192k -shortest "$dir/$name.mp4"
rm -f "$home/replay/$name.mp4"
echo "$dir/$name.mp4"
