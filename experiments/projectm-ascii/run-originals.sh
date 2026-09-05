#!/bin/bash
set -euo pipefail
here=$(dirname "$(readlink -f "$0")")
root=$(readlink -f "$here/../..")
preset_dir="$root/presets/milkdrop-originals"
[[ -r $root/lib/libprojectM-4.so.4.1.7 ]] || {
  echo 'omadrop: build the original-preset runtime with bin/build-milkdrop-runtime' >&2
  exit 1
}
export LD_LIBRARY_PATH="$root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export OMADROP_ENGINE=projectm OMADROP_MILKDROP_ORIGINALS=1
presets=()
while IFS= read -r name; do
  [[ -n $name && $name != */* && -f $preset_dir/$name ]] || {
    echo "omadrop: missing original preset: $name" >&2; exit 1;
  }
  presets+=("$preset_dir/$name")
done < "$preset_dir/playlist.txt"
((${#presets[@]} >= 20)) || { echo 'omadrop: incomplete original collection' >&2; exit 1; }
exec "$here/projectm-ascii-live" "${presets[@]}"
