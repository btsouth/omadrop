#!/bin/bash
# Build and install Omadrop for the current user.
#
# One product, one root: the MilkDrop collection and the Omarchy music
# screensaver, driven by the single public `omadrop` command.
set -euo pipefail

root=$(dirname "$(readlink -f "$0")")
data_home=${XDG_DATA_HOME:-$HOME/.local/share}
config_home=${XDG_CONFIG_HOME:-$HOME/.config}
install_root=${OMADROP_INSTALL_ROOT:-$data_home/omadrop}
bin_dir=${OMADROP_BIN_DIR:-$HOME/.local/bin}
install_dependencies=1
install_bindings=1
prebuilt=
stage_dir=

bin_scripts=(
  omadrop omadrop-milkdrop omadrop-effects omadrop-screensaver omadrop-screensaver-run
  mpris-state mpris-art art-fetch art-prep omadrop-doctor omadrop-close-window
  omadrop-calibrate omadrop-pack omadrop-demo omadrop-demo-record omadrop-preview
  demo-audio-audit demo-rights-audit
)
projectm_scripts=(run-collection.sh run-originals.sh run-curated.sh)
projectm_tools=(gpu-probe scene-pack-audit scene-pack-author audio-match)

usage() {
  cat <<'EOF'
Usage: ./install.sh [--no-deps] [--no-bindings] [--prebuilt DIR] [--stage DIR]

Build and install Omadrop for the current user: the MilkDrop collection and the
Omarchy music screensaver, driven by the single `omadrop` command.

  --no-deps       Do not install missing Arch packages
  --no-bindings   Do not add Omarchy keyboard shortcuts
  --prebuilt DIR  Install an already staged root instead of building
  --stage DIR     Build and copy the root layout into DIR without installing
EOF
}

while (($#)); do
  case $1 in
    --no-deps) install_dependencies=0 ;;
    --no-bindings) install_bindings=0 ;;
    --prebuilt)
      (($# >= 2)) || { echo "install: --prebuilt requires a directory" >&2; exit 2; }
      prebuilt=$2
      shift
      ;;
    --stage)
      (($# >= 2)) || { echo "install: --stage requires a directory" >&2; exit 2; }
      stage_dir=$2
      install_dependencies=0
      install_bindings=0
      shift
      ;;
    -h|--help) usage; exit 0 ;;
    *) echo "install: unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
  shift
done

[[ $install_root == /* && $bin_dir == /* && $config_home == /* ]] || {
  echo "install: XDG and installation paths must be absolute" >&2
  exit 1
}

build_all() {
  "$root/bin/build-install-runtime"
  ( cd "$root/screensaver/ttfx" && cargo build --release --locked )
  ( mkdir -p "$root/app/build" && cd "$root/app/build" \
      && qmake6 ../omadrop-ui.pro && make -j"$(nproc)" )
}

stage_from_repo() {
  local dest=$1 name file
  install -d "$dest/bin" "$dest/lib" "$dest/experiments/projectm-ascii" \
    "$dest/presets/pilot" "$dest/shaders/native" "$dest/scene-api/1" "$dest/licenses"
  for name in "${bin_scripts[@]}"; do
    install -Dm755 "$root/bin/$name" "$dest/bin/$name"
  done
  install -Dm755 "$root/screensaver/ttfx/target/release/ttfx" "$dest/bin/ttfx-music"
  install -Dm755 "$root/app/build/omadrop-ui" "$dest/bin/omadrop-ui"
  cp -a "$root"/lib/libprojectM-4.so* "$dest/lib/"
  for name in "${projectm_scripts[@]}" "${projectm_tools[@]}"; do
    install -Dm755 "$root/experiments/projectm-ascii/$name" \
      "$dest/experiments/projectm-ascii/$name"
  done
  install -Dm755 "$root/experiments/projectm-ascii/projectm-ascii-live" \
    "$dest/experiments/projectm-ascii/projectm-ascii-live"
  cp -a "$root/presets/pilot/." "$dest/presets/pilot/"
  install -Dm644 "$root/presets/pilot.txt" "$dest/presets/pilot.txt"
  cp -a "$root/presets/textures" "$dest/presets/"
  cp -a "$root/presets/captions" "$dest/presets/"
  cp -a "$root/presets/milkdrop-originals" "$dest/presets/"
  install -Dm644 "$root/experiments/milkdrop-audio-pilot/manifest.json" \
    "$dest/presets/collection-manifest.json"
  for file in "$root"/shaders/native/*.vert "$root"/shaders/native/*.glsl \
      "$root"/shaders/native/*.frag; do
    [[ -e $file ]] || continue
    install -Dm644 "$file" "$dest/shaders/native/$(basename "$file")"
  done
  for file in "$root"/scene-api/1/*.vert "$root"/scene-api/1/*.glsl; do
    [[ -e $file ]] || continue
    install -Dm644 "$file" "$dest/scene-api/1/$(basename "$file")"
  done
  install -Dm644 "$root/LICENSE" "$dest/licenses/LICENSE"
  install -Dm644 "$root/THIRD_PARTY_NOTICES.md" "$dest/licenses/THIRD_PARTY_NOTICES.md"
  install -Dm644 "$root/screensaver/ttfx/LICENSE" "$dest/licenses/ttfx-LICENSE"
  install -Dm644 "$root/screensaver/ttfx/NOTICE" "$dest/licenses/ttfx-NOTICE"
  for file in "$root"/third-party/projectm/*; do
    install -Dm644 "$file" "$dest/licenses/projectm-$(basename "$file")"
  done
  install -Dm644 "$root/VERSION" "$dest/VERSION"
}

stage_prebuilt() {
  local source=$1 dest=$2
  [[ -d $source ]] || { echo "install: --prebuilt directory not found: $source" >&2; exit 1; }
  [[ -x $source/bin/omadrop-milkdrop ]] || {
    echo "install: --prebuilt is not a staged Omadrop root: $source" >&2; exit 1;
  }
  cp -a "$source/." "$dest/"
}

if [[ -n $stage_dir ]]; then
  [[ $stage_dir == /* ]] || { echo "install: --stage path must be absolute" >&2; exit 1; }
  mkdir -p "$stage_dir"
  if [[ -n $prebuilt ]]; then
    stage_prebuilt "$prebuilt" "$stage_dir"
  else
    build_all
    stage_from_repo "$stage_dir"
  fi
  echo "Staged Omadrop in $stage_dir"
  exit 0
fi

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

dependencies=(
  gcc pkgconf cmake ninja git python glslang sdl2-compat glew libpng fftw
  json-c pipewire-audio libpulse imagemagick curl glib2 jq
  ffmpeg gpu-screen-recorder rust qt6-base qt6-declarative
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

[[ -n $prebuilt ]] || build_all

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
      if [[ -L $bin_dir/omadrop && $(readlink -m "$bin_dir/omadrop") == "$final_root/"* && ! -e $bin_dir/omadrop ]]; then
        unlink "$bin_dir/omadrop"
      fi
    fi
    [[ ! -d $staging_root ]] || rm -rf -- "$staging_root"
  fi
  return "$status"
}
trap cleanup_install EXIT

if [[ -n $prebuilt ]]; then
  stage_prebuilt "$prebuilt" "$install_root"
else
  stage_from_repo "$install_root"
fi

# Stage checks run before any installed file is replaced.
[[ -x $install_root/bin/omadrop-ui ]] || { echo 'install: the Qt controller was not built' >&2; exit 1; }
[[ -x $install_root/bin/ttfx-music ]] || { echo 'install: ttfx-music was not built' >&2; exit 1; }
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

# One command symlink. A foreign `omadrop` file is backed up exactly once; an
# older Omadrop symlink (including the legacy product root) is repointed.
install_command() {
  local name link target backup
  mkdir -p "$bin_dir"
  for name in omadrop-preview omadrop-close-window omadrop-demo omadrop-demo-record omadrop-doctor; do
    link=$bin_dir/$name
    [[ -L $link ]] || continue
    target=$(readlink -m "$link")
    case $target in
      "$final_root"/*|"$data_home/omadrop-product"/*|"$data_home/omadrop-screensaver"/*) unlink "$link" ;;
    esac
  done
  if [[ -e $bin_dir/omadrop && ! -L $bin_dir/omadrop ]]; then
    backup=$bin_dir/omadrop.before-omadrop
    if [[ ! -e $backup && ! -L $backup ]]; then
      mv -- "$bin_dir/omadrop" "$backup"
    fi
  fi
  ln -sfn "$install_root/bin/omadrop" "$bin_dir/omadrop"
}
install_command

install_desktop() {
  local entry path
  mkdir -p "$data_home/applications" "$data_home/icons/hicolor/scalable/apps"
  install -Dm644 "$root/packaging/omadrop.desktop" "$data_home/applications/omadrop.desktop"
  install -Dm644 "$root/packaging/omadrop.svg" "$data_home/icons/hicolor/scalable/apps/omadrop.svg"
  # Retire the earlier two-launcher product entries; this install owns one.
  for entry in omadrop-modes.desktop omadrop-effects.desktop; do
    path=$data_home/applications/$entry
    [[ -f $path ]] || continue
    grep -Eq '^(Exec|TryExec)=.*omadrop' "$path" && rm -f -- "$path"
  done
}
install_desktop

install_omarchy_bindings() {
  local bindings=$config_home/hypr/bindings.lua
  local keybindings shift_binding alt_binding backup temporary escaped_command escaped_close
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
  escaped_close=${install_root//\\/\\\\}
  escaped_close=${escaped_close//\"/\\\"}
  cat >> "$temporary" <<EOF

-- omadrop:bindings:start
hl.unbind("SUPER + SHIFT + V")
hl.unbind("SUPER + W")
hl.unbind("SUPER + ALT + V")
o.bind("SUPER + SHIFT + V", "Omadrop", "$escaped_command/omadrop")
o.bind("SUPER + W", "Close window", "$escaped_close/bin/omadrop-close-window")
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
