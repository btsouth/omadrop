#!/bin/bash
# Run only inside a live Hyprland session. This opens the collection briefly.
set -euo pipefail
root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
hyprctl monitors -j | jq -e 'length > 0' >/dev/null
if hyprctl clients -j | jq -e 'any(.[]; .class == "org.omadrop.milkdrop" or .class == "org.omadrop.screensaver")' >/dev/null; then
  echo 'Close existing Omadrop visuals before measuring.' >&2
  exit 2
fi
log=$(mktemp)
trap 'rm -f "$log"' EXIT
OMADROP_TIMING=1 OMADROP_AUTO_QUIT_MS=2000 OMADROP_PERSIST_DISPLAY=0 \
  timeout --kill-after=2s 8s "$root/bin/omadrop" --mode milkdrop --single >"$log" 2>&1

printf '%-24s %8s\n' 'Stage' 'ms'
declare -A timings=()
while IFS=$'\t' read -r kind ms stage; do
  [[ $kind == timing && $ms =~ ^[0-9]+$ ]] || continue
  if [[ -z ${timings[$stage]+present} ]]; then
    timings[$stage]=$ms
    printf '%-24s %8s\n' "$stage" "$ms"
  fi
done <"$log"
for stage in 'dispatcher start' 'renderer exec' 'GL context ready' \
  'first preset loaded' 'first frame presented' 'window shown'; do
  if [[ -z ${timings[$stage]+present} ]]; then
    echo "Missing timing stage: $stage" >&2
    cat "$log" >&2
    exit 1
  fi
done
if ((timings['window shown'] > 1500)); then
  echo 'Window shown exceeded 1500 ms.' >&2
  exit 1
fi
