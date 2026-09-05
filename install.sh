#!/bin/bash
set -euo pipefail

root=$(dirname "$(readlink -f "$0")")
install_root=${OMADROP_INSTALL_ROOT:-${XDG_DATA_HOME:-$HOME/.local/share}/omadrop}
bin_dir=${OMADROP_BIN_DIR:-$HOME/.local/bin}
config_home=${XDG_CONFIG_HOME:-$HOME/.config}
install_dependencies=1
install_bindings=1

usage() {
  cat <<'EOF'
Usage: ./install.sh [--no-deps] [--no-bindings]

Build and install Omadrop for the current user.

  --no-deps      Do not install missing Arch packages
  --no-bindings  Do not add Omarchy keyboard shortcuts
EOF
}

while (($#)); do
  case $1 in
    --no-deps) install_dependencies=0 ;;
    --no-bindings) install_bindings=0 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "install: unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
  shift
done

[[ $install_root == /* && $bin_dir == /* && $config_home == /* ]] || {
  echo "install: XDG and installation paths must be absolute" >&2
  exit 1
}

[[ $install_root != / && $install_root != "$HOME" && $install_root != "$root" && ! -L $install_root ]] || {
  echo 'install: unsafe installation root' >&2; exit 1;
}
mkdir -p "$(dirname "$install_root")"
exec {install_lock_fd}>"${install_root}.install.lock"
flock -n "$install_lock_fd" || { echo 'install: another installation is running' >&2; exit 1; }
if [[ -e $install_root && ! -f $install_root/VERSION ]]; then
  echo 'install: target is not an existing Omadrop installation' >&2; exit 1
fi
if pgrep -f "^${install_root//./\\.}/experiments/projectm-ascii/projectm-ascii-live( |$)" >/dev/null; then
  echo 'install: close Omadrop before updating' >&2; exit 1
fi
commands=(omadrop omadrop-preview omadrop-close-window omadrop-demo omadrop-demo-record omadrop-doctor)
for command in "${commands[@]}"; do
  path=$bin_dir/$command
  if [[ -e $path || -L $path ]]; then
    [[ -L $path && $(readlink -m "$path") == "$install_root/"* ]] || {
      echo "install: refusing to replace unrelated command $path" >&2; exit 1;
    }
  fi
done

dependencies=(
  gcc pkgconf cmake ninja git python glslang libprojectm sdl2-compat glew libpng fftw
  json-c pipewire-audio libpulse imagemagick curl glib2 jq
  ffmpeg gpu-screen-recorder
)
if ((install_dependencies)); then
  if command -v omarchy >/dev/null; then
    omarchy pkg add "${dependencies[@]}"
  elif command -v pacman >/dev/null; then
    sudo pacman -S --needed -- "${dependencies[@]}"
  else
    echo "install: Arch package manager not found; install dependencies from README.md" >&2
    exit 1
  fi
fi

"$root/bin/build-install-runtime"

# Assemble the complete runtime beside the target, then promote it.
final_root=$install_root
mkdir -p "$(dirname "$final_root")"
install_root=$(mktemp -d "${final_root}.staging.XXXXXX")
staging_root=$install_root
backup_root=
promoted=0
transaction_done=0
cleanup_install() {
  local status=$?
  if ((transaction_done == 0)); then
    if ((promoted)); then
      rm -rf -- "$final_root"
      [[ -z $backup_root ]] || mv -- "$backup_root" "$final_root"
      for command in "${commands[@]}"; do
        if [[ -L $bin_dir/$command && $(readlink -m "$bin_dir/$command") == "$final_root/"* && ! -e $bin_dir/$command ]]; then
          unlink "$bin_dir/$command"
        fi
      done
    fi
    [[ ! -d $staging_root ]] || rm -rf -- "$staging_root"
  fi
  return "$status"
}
trap cleanup_install EXIT

install -Dm755 "$root/bin/omadrop" "$install_root/bin/omadrop"
install -Dm755 "$root/bin/omadrop-preview" "$install_root/bin/omadrop-preview"
install -Dm755 "$root/bin/omadrop-close-window" "$install_root/bin/omadrop-close-window"
install -Dm755 "$root/bin/omadrop-calibrate" "$install_root/bin/omadrop-calibrate"
install -Dm755 "$root/bin/omadrop-demo" "$install_root/bin/omadrop-demo"
install -Dm755 "$root/bin/omadrop-demo-record" "$install_root/bin/omadrop-demo-record"
install -Dm755 "$root/bin/demo-audio-audit" "$install_root/bin/demo-audio-audit"
install -Dm755 "$root/bin/demo-rights-audit" "$install_root/bin/demo-rights-audit"
install -Dm755 "$root/bin/omadrop-doctor" "$install_root/bin/omadrop-doctor"
install -Dm755 "$root/bin/omadrop-pack" "$install_root/bin/omadrop-pack"
install -Dm755 "$root/bin/mpris-art" "$install_root/bin/mpris-art"
install -Dm755 "$root/bin/mpris-state" "$install_root/bin/mpris-state"
install -Dm755 "$root/bin/art-fetch" "$install_root/bin/art-fetch"
install -Dm755 "$root/bin/art-prep" "$install_root/bin/art-prep"
install -Dm755 "$root/experiments/projectm-ascii/projectm-ascii-live" \
  "$install_root/experiments/projectm-ascii/projectm-ascii-live"
install -Dm755 "$root/experiments/projectm-ascii/audio-match" \
  "$install_root/experiments/projectm-ascii/audio-match"
install -Dm755 "$root/experiments/projectm-ascii/scene-pack-audit" \
  "$install_root/experiments/projectm-ascii/scene-pack-audit"
install -Dm755 "$root/experiments/projectm-ascii/scene-pack-author" \
  "$install_root/experiments/projectm-ascii/scene-pack-author"
install -Dm755 "$root/experiments/projectm-ascii/gpu-probe" \
  "$install_root/experiments/projectm-ascii/gpu-probe"
install -Dm755 "$root/experiments/projectm-ascii/run-curated.sh" \
  "$install_root/experiments/projectm-ascii/run-curated.sh"
for shader in "$root"/shaders/native/*.{vert,glsl,frag}; do
  install -Dm644 "$shader" "$install_root/shaders/native/$(basename "$shader")"
done
for api_file in "$root"/scene-api/1/*.{vert,glsl}; do
  install -Dm644 "$api_file" \
    "$install_root/scene-api/1/$(basename "$api_file")"
done
install -Dm755 "$root/experiments/projectm-ascii/run-originals.sh"   "$install_root/experiments/projectm-ascii/run-originals.sh"
install -d "$install_root/third-party/projectm"
cp -a "$root/third-party/projectm/." "$install_root/third-party/projectm/"
install -d "$install_root/lib" "$install_root/presets/milkdrop-originals" "$install_root/presets/textures"
cp -a "$root"/lib/libprojectM-4.so* "$install_root/lib/"
cp -a "$root/presets/milkdrop-originals/." "$install_root/presets/milkdrop-originals/"
cp -a "$root/presets/textures/." "$install_root/presets/textures/"
install -d "$install_root/presets/pilot"
cp -a "$root/presets/pilot/." "$install_root/presets/pilot/"
install -Dm644 "$root/presets/pilot.txt" "$install_root/presets/pilot.txt"
install -Dm755 "$root/experiments/projectm-ascii/run-collection.sh" "$install_root/experiments/projectm-ascii/run-collection.sh"
install -Dm644 "$root/experiments/milkdrop-audio-pilot/manifest.json" "$install_root/presets/collection-manifest.json"
install -d "$install_root/presets/curated"
rm -f "$install_root/presets/curated/A New Definition for Milk - AdamFX - Laser Show in a Crystalstorm  ft Orb n Martin Inside the Forge of Isengard.milk"
rm -f "$install_root/presets/curated/shifter - lattice (eclipse) Phat + EoS more color mix_v2.milk"
rm -f "$install_root/presets/curated/Aderrasi - Contortion (Escher's Tunnel Mix).milk"
rm -f "$install_root/presets/curated/Aderrasi - Halls Of Centrifuge.milk"
rm -f "$install_root/presets/curated/EoS + Phat - cubetrace - v2.milk"
rm -f "$install_root/presets/curated/Geiss - Myriad Mosaics.milk"
rm -f "$install_root/presets/curated/Martin - wire dance.milk"
rm -f "$install_root/presets/curated/Phat+fiShbRaiN+EoS_Mandala_Chasers_remix.milk"
rm -f "$install_root/presets/curated/The NG + Geiss + Flexi - The Waterfowl In The Rain.milk"
rm -f "$install_root/presets/curated/shifter - mandala.milk"
install -m644 "$root"/presets/curated/*.milk "$install_root/presets/curated/"
install -Dm644 "$root/presets/classic/Aderrasi - Contortion (Escher's Tunnel Mix).milk" \
  "$install_root/presets/classic/Aderrasi - Contortion (Escher's Tunnel Mix).milk"
install -Dm644 "$root/presets/classic/Aderrasi - Halls Of Centrifuge.milk" \
  "$install_root/presets/classic/Aderrasi - Halls Of Centrifuge.milk"
install -Dm644 "$root/presets/classic/Martin - wire dance.milk" \
  "$install_root/presets/classic/Martin - wire dance.milk"
install -Dm755 "$root/uninstall.sh" "$install_root/uninstall.sh"
install -Dm644 "$root/README.md" "$install_root/README.md"
install -Dm644 "$root/CHANGELOG.md" "$install_root/CHANGELOG.md"
install -Dm644 "$root/LICENSE" "$install_root/LICENSE"
install -Dm644 "$root/docs/legacy-preset-notices.md" "$install_root/docs/legacy-preset-notices.md"
install -Dm644 "$root/THIRD_PARTY_NOTICES.md" "$install_root/THIRD_PARTY_NOTICES.md"
install -Dm644 "$root/VERSION" "$install_root/VERSION"
install -Dm644 "$root/docs/controls.md" "$install_root/docs/controls.md"
install -Dm644 "$root/docs/troubleshooting.md" \
  "$install_root/docs/troubleshooting.md"
install -Dm644 "$root/docs/scene-packs.md" "$install_root/docs/scene-packs.md"
install -Dm644 "$root/docs/scene-pack-v1.schema.json" \
  "$install_root/docs/scene-pack-v1.schema.json"
install -Dm644 "$root/docs/scene-pack-v2.schema.json" \
  "$install_root/docs/scene-pack-v2.schema.json"
install -Dm644 "$root/docs/demo-music.md" "$install_root/docs/demo-music.md"
install -Dm644 "$root/docs/accessibility-testing.md" \
  "$install_root/docs/accessibility-testing.md"
install -Dm644 "$root/docs/real-song-audit.md" \
  "$install_root/docs/real-song-audit.md"
install -Dm644 "$root/demo/music-rights.json" \
  "$install_root/demo/music-rights.json"
install -Dm644 "$root/demo/scene-sequence.txt" \
  "$install_root/demo/scene-sequence.txt"
install -Dm644 "$root/site/public/og.png" \
  "$install_root/site/public/og.png"
# Stage checks run before any installed file is replaced.
[[ -x $install_root/experiments/projectm-ascii/projectm-ascii-live ]]
[[ $(wc -l < "$install_root/presets/pilot.txt") == 21 ]]
if [[ -e $final_root ]]; then
  backup_root=$(mktemp -d "${final_root}.previous.XXXXXX")
  rmdir "$backup_root"
  mv -- "$final_root" "$backup_root"
fi
if ! mv -- "$staging_root" "$final_root"; then
  [[ -z $backup_root ]] || mv -- "$backup_root" "$final_root"
  exit 1
fi
promoted=1
install_root=$final_root
mkdir -p "$bin_dir"
ln -sfn "$install_root/bin/omadrop" "$bin_dir/omadrop"
ln -sfn "$install_root/bin/omadrop-preview" "$bin_dir/omadrop-preview"
ln -sfn "$install_root/bin/omadrop-close-window" "$bin_dir/omadrop-close-window"
ln -sfn "$install_root/bin/omadrop-demo" "$bin_dir/omadrop-demo"
ln -sfn "$install_root/bin/omadrop-demo-record" "$bin_dir/omadrop-demo-record"
ln -sfn "$install_root/bin/omadrop-doctor" "$bin_dir/omadrop-doctor"

install_omarchy_bindings() {
  local bindings=$config_home/hypr/bindings.lua
  local keybindings shift_binding alt_binding backup temporary escaped_command
  keybindings=$(omarchy menu keybindings --print 2>/dev/null || true)
  shift_binding=$(grep -E '^SUPER SHIFT \+ V[[:space:]]' <<<"$keybindings" | head -n 1 || true)
  alt_binding=$(grep -E '^SUPER ALT \+ V[[:space:]]' <<<"$keybindings" | head -n 1 || true)
  if [[ -n $shift_binding && $shift_binding != *'→ Omadrop'*
        || -n $alt_binding && $alt_binding != *'→ Toggle Omadrop secondary display'* ]]; then
    echo "install: Omadrop installed, but shortcuts were skipped because one is already in use:" >&2
    [[ -n $shift_binding ]] && echo "  $shift_binding" >&2
    [[ -n $alt_binding ]] && echo "  $alt_binding" >&2
    return
  fi

  mkdir -p "$(dirname "$bindings")"
  touch "$bindings"
  backup=$bindings.bak.$(date +%s)
  cp -p "$bindings" "$backup"
  temporary=$(mktemp "$(dirname "$bindings")/.omadrop-bindings.XXXXXX")
  awk '
    /^-- omadrop:bindings:start$/ { managed = 1; next }
    /^-- omadrop:bindings:end$/ { managed = 0; next }
    managed { next }
    /o\.bind\("SUPER \+ SHIFT \+ V", "Omadrop",/ { next }
    /o\.bind\("SUPER \+ ALT \+ V", "Toggle Omadrop secondary display",/ { next }
    { print }
  ' "$bindings" > "$temporary"
  escaped_command=${bin_dir//\\/\\\\}
  escaped_command=${escaped_command//\"/\\\"}
  cat >> "$temporary" <<EOF

-- omadrop:bindings:start
hl.unbind("SUPER + SHIFT + V")
hl.unbind("SUPER + W")
hl.unbind("SUPER + ALT + V")
o.bind("SUPER + SHIFT + V", "Omadrop", "$escaped_command/omadrop")
o.bind("SUPER + W", "Close window", "$escaped_command/omadrop-close-window")
o.bind("SUPER + ALT + V", "Toggle Omadrop secondary display", "$escaped_command/omadrop --toggle-secondary")
o.window("projectm-ascii-live", { idle_inhibit = "always" })
-- omadrop:bindings:end
EOF
  chmod --reference="$bindings" "$temporary"
  mv "$temporary" "$bindings"

  if command -v hyprctl >/dev/null && [[ ${OMADROP_SKIP_HYPR_RELOAD:-0} != 1 ]]; then
    hyprctl reload >/dev/null
    local errors
    errors=$(hyprctl configerrors 2>&1 || true)
    if [[ -n $errors ]]; then
      cp -p "$backup" "$bindings"
      hyprctl reload >/dev/null || true
      echo "install: Hyprland rejected the shortcuts; restored $backup" >&2
      echo "$errors" >&2
      exit 1
    fi
  fi
}

if ((install_bindings)); then
  if command -v omarchy >/dev/null; then
    install_omarchy_bindings
  else
    echo "install: Omarchy not found; shortcuts were not installed" >&2
  fi
fi

transaction_done=1
[[ -z $backup_root ]] || echo "Previous installation preserved: $backup_root"
version=$(<"$root/VERSION")
echo "Omadrop $version installed in $install_root"
echo "Run: $bin_dir/omadrop"
if [[ :$PATH: != *":$bin_dir:"* ]]; then
  echo "Add $bin_dir to PATH to run omadrop by name."
fi
