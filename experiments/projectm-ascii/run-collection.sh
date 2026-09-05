#!/bin/bash
set -euo pipefail
root=$(cd "$(dirname "$(readlink -f "$0")")/../.." && pwd)
while (($#)); do
  case $1 in
    --original) export OMADROP_PILOT_ORIGINAL=1 ;;
    *) echo "omadrop: unknown option: $1" >&2; exit 2 ;;
  esac
  shift
done
export LD_LIBRARY_PATH="$root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export OMADROP_ENGINE=projectm OMADROP_MILKDROP_ORIGINALS=1
export OMADROP_ASCII=${OMADROP_ASCII:-0}
export OMADROP_HIDE_FIRST_RUN_CONTROLS=1
export OMADROP_COVER_HOLD_SECONDS=3.5 OMADROP_COVER_DISSOLVE_SECONDS=1.75
presets=()
while IFS= read -r name; do
  [[ -n "$name" && "$name" != */* && -f "$root/presets/pilot/$name" ]] || {
    echo 'omadrop: incomplete collection; reinstall Omadrop' >&2; exit 1;
  }
  presets+=("$root/presets/pilot/$name")
done < "$root/presets/pilot.txt"
((${#presets[@]} == 21)) || { echo 'omadrop: expected 21 presets' >&2; exit 1; }
exec "${OMADROP_LIVE_EXECUTABLE:-$root/experiments/projectm-ascii/projectm-ascii-live}" "${presets[@]}"
