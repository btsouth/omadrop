#!/bin/bash
set -euo pipefail
root=$(cd "$(dirname "$(readlink -f "$0")")/../.." && pwd)
pilot="$root/cache/milkdrop-audio-pilot/runtime"
if [[ -x "$root/experiments/projectm-ascii/projectm-audio-pilot" ]]; then pilot="$root"; fi
[[ -x "$pilot/experiments/projectm-ascii/projectm-audio-pilot" ]] || {
  echo 'Build first: experiments/milkdrop-audio-pilot/build.sh' >&2; exit 1;
}
collection=pilot
while (($#)); do
  case $1 in
    --original) export OMADROP_PILOT_ORIGINAL=1 ;;
    --all) collection=rotation ;;
    --help) echo 'Omadrop 21-scene collection: O compare response, N next, P previous, F11 fullscreen, Esc close. --original disables additions; --all uses the original playlist order.'; exit 0 ;;
    *) echo "Unknown argument: $1" >&2; exit 2 ;;
  esac
  shift
done
export LD_LIBRARY_PATH="$pilot/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export OMADROP_ENGINE=projectm OMADROP_MILKDROP_ORIGINALS=1 OMADROP_ASCII=0
export OMADROP_HIDE_FIRST_RUN_CONTROLS=1 OMADROP_COVER_HOLD_SECONDS=3.5 OMADROP_COVER_DISSOLVE_SECONDS=1.75
# Separate preferences and a single window keep this comparison independent of the baseline.
config_base=${XDG_CONFIG_HOME:-$HOME/.config}
pilot_config=${OMADROP_PILOT_CONFIG:-$config_base/omadrop-audio-pilot}
if [[ ! -d "$pilot_config/omadrop" ]]; then
  mkdir -p "$pilot_config/omadrop"
  if [[ -d "$config_base/omadrop/sync-by-sink" ]]; then
    cp -a "$config_base/omadrop/sync-by-sink" "$pilot_config/omadrop/"
  fi
fi
export XDG_CONFIG_HOME="$pilot_config"
unset OMADROP_PAIR_ROLE OMADROP_PAIR_STATE OMADROP_READY_FILE OMADROP_START_GATE
unset OMADROP_NATIVE_SCENE OMADROP_MUSICAL_PREVIEW OMADROP_COLLECTION_PREVIEW
playlist="$pilot/presets/$collection.txt"
[[ -r "$playlist" ]] || { echo 'Missing pilot playlist.' >&2; exit 1; }
presets=()
while IFS= read -r name; do
  [[ -n "$name" && "$name" != */* ]] || { echo 'Invalid preset name.' >&2; exit 1; }
  path="$pilot/presets/pilot/$name"
  if [[ ! -f "$path" ]]; then path="$pilot/presets/milkdrop-originals/$name"; fi
  [[ -f "$path" ]] || { echo "Missing preset: $name" >&2; exit 1; }
  presets+=("$path")
done < "$playlist"
expected=21
[[ $collection == rotation ]] && expected=21
((${#presets[@]} == expected)) || { echo 'Incomplete collection.' >&2; exit 1; }
exec "$pilot/experiments/projectm-ascii/projectm-audio-pilot" "${presets[@]}"
