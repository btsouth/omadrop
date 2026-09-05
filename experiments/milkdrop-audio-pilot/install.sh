#!/bin/bash
set -euo pipefail
root=$(cd "$(dirname "$0")/../.." && pwd)
source_runtime="$root/cache/milkdrop-audio-pilot/runtime"
target="$HOME/.local/share/omadrop-audio-pilot"
[[ -x "$source_runtime/experiments/projectm-ascii/projectm-audio-pilot" ]] || { echo 'Build the pilot first.' >&2; exit 1; }
if pgrep -f "^${target//./\.}/experiments/projectm-ascii/projectm-audio-pilot( |$)" >/dev/null; then
  echo 'Close the audio pilot before updating it. The baseline may stay open.' >&2
  exit 1
fi
mkdir -p "$target" "$HOME/.local/bin"
# Never install into the accepted baseline or change its launcher.
cp -a "$source_runtime/." "$target/"
mkdir -p "$target/experiments/milkdrop-audio-pilot" "$target/third-party"
cp "$root/experiments/milkdrop-audio-pilot/"{run.sh,manifest.json,README.md} "$target/experiments/milkdrop-audio-pilot/"
cp -a "$root/third-party/." "$target/third-party/"
ln -sfn "$target/experiments/milkdrop-audio-pilot/run.sh" "$HOME/.local/bin/omadrop-audio-pilot"
echo 'Installed separately. Launch: omadrop-audio-pilot'
