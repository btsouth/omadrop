#!/bin/bash
set -euo pipefail
root=$(cd "$(dirname "$(readlink -f "$0")")/../.." && pwd)

scene=
scene_set=0
while (($#)); do
  case $1 in
    --original) export OMADROP_PILOT_ORIGINAL=1 ;;
    --scene)
      (($# >= 2)) || { echo "omadrop: --scene requires a scene number" >&2; exit 2; }
      [[ $2 =~ ^[1-9][0-9]*$ ]] || { echo "omadrop: --scene requires a scene number" >&2; exit 2; }
      scene=$2
      scene_set=1
      shift
      ;;
    *) echo "omadrop: unknown option: $1" >&2; exit 2 ;;
  esac
  shift
done

export LD_LIBRARY_PATH="$root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export OMADROP_COLLECTION_MANIFEST="$root/presets/collection-manifest.json"
export OMADROP_ENGINE=projectm OMADROP_MILKDROP_ORIGINALS=1
# Start new profiles in continuous mode, but honor saved and explicit choices.
if [[ -z ${OMADROP_ASCII+x} ]]; then
  config=${XDG_CONFIG_HOME:-$HOME/.config}/omadrop
  if ! grep -Eq '^ascii=[01]$' "$config/preferences.conf" 2>/dev/null \
      && [[ ! -f "$config/ascii-enabled" ]]; then
    export OMADROP_ASCII=0
  fi
fi
export OMADROP_HIDE_FIRST_RUN_CONTROLS=1
export OMADROP_COVER_HOLD_SECONDS=3.5 OMADROP_COVER_DISSOLVE_SECONDS=1.75

names=()
while IFS= read -r name; do
  [[ -n "$name" && "$name" != */* && -f "$root/presets/pilot/$name" ]] || {
    echo 'omadrop: incomplete collection; reinstall Omadrop' >&2; exit 1;
  }
  names+=("$name")
done < "$root/presets/pilot.txt"
((${#names[@]} == 21)) || { echo 'omadrop: expected 21 presets' >&2; exit 1; }

if ((scene_set)) && ((scene < 1 || scene > ${#names[@]})); then
  echo "omadrop: scene number out of range: $scene" >&2
  exit 2
fi

# Scene selection: omadrop/scenes.conf hides collection scene numbers. A missing,
# garbled or fully hidden file falls back to the whole collection.
scenes_conf=${XDG_CONFIG_HOME:-$HOME/.config}/omadrop/scenes.conf
hidden=()
full_collection=0
if [[ -r $scenes_conf ]]; then
  version=$(awk -F= '$1 == "version" { print $2; exit }' "$scenes_conf" 2>/dev/null || true)
  raw=$(awk -F= '$1 == "hidden" { print $2; exit }' "$scenes_conf" 2>/dev/null || true)
  if [[ $version != 1 ]]; then
    full_collection=1
  elif [[ -n $raw ]]; then
    IFS=, read -r -a tokens <<<"$raw"
    for token in "${tokens[@]}"; do
      token=${token//[[:space:]]/}
      if [[ ! $token =~ ^[1-9][0-9]*$ ]] \
          || ((token < 1 || token > ${#names[@]})); then
        full_collection=1
        break
      fi
      hidden+=("$token")
    done
    if ((full_collection == 0 && ${#hidden[@]} > 0)); then
      declare -A seen=()
      for token in "${hidden[@]}"; do seen[$token]=1; done
      ((${#seen[@]} >= ${#names[@]})) && full_collection=1
    fi
  fi
fi
((full_collection)) && hidden=()

is_hidden() {
  local token
  for token in "${hidden[@]}"; do
    [[ $token == "$1" ]] && return 0
  done
  return 1
}

# Preserve collection order. A hidden scene stays out unless --scene names it,
# so a requested scene is always present and becomes the start preset.
presets=()
start_index=
for ((index = 0; index < ${#names[@]}; index++)); do
  number=$((index + 1))
  requested=0
  ((scene_set && scene == number)) && requested=1
  if ((full_collection == 0 && requested == 0)) && is_hidden "$number"; then
    continue
  fi
  presets+=("$root/presets/pilot/${names[index]}")
  if ((requested)); then
    start_index=$(( ${#presets[@]} - 1 ))
  fi
done
((${#presets[@]} >= 1)) || { echo 'omadrop: no scenes to display' >&2; exit 1; }

if ((scene_set)); then
  export OMADROP_START_PRESET=$start_index
fi

exec "${OMADROP_LIVE_EXECUTABLE:-$root/experiments/projectm-ascii/projectm-ascii-live}" "${presets[@]}"
