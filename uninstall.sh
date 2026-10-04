#!/bin/bash
# Remove the per-user Omadrop installation created by install.sh.
#
# Removes the installed root, the one `omadrop` command and the Omadrop desktop
# entry and icon. Settings, cached artwork and legacy data directories
# are preserved.
set -euo pipefail

data_home=${XDG_DATA_HOME:-$HOME/.local/share}
config_home=${XDG_CONFIG_HOME:-$HOME/.config}
install_root=${OMADROP_INSTALL_ROOT:-$data_home/omadrop}
bin_dir=${OMADROP_BIN_DIR:-$HOME/.local/bin}

script_path=$(readlink -f "$0")
if [[ $script_path == "$install_root"/* && ${OMADROP_UNINSTALL_REEXEC:-0} != 1 ]]; then
  temporary_self=$(mktemp /tmp/omadrop-uninstall.XXXXXX)
  cp "$script_path" "$temporary_self"
  chmod 700 "$temporary_self"
  OMADROP_UNINSTALL_REEXEC=1 exec "$temporary_self" "$@"
fi
if [[ ${OMADROP_UNINSTALL_REEXEC:-0} == 1 ]]; then
  trap 'unlink "$script_path"' EXIT
fi

[[ $install_root == /* && -n $install_root && $install_root != / && $install_root != "$HOME" ]] || {
  echo "uninstall: refusing unsafe installation path: $install_root" >&2
  exit 1
}

remove_link() {
  local link=$1 target
  [[ -L $link ]] || return 0
  target=$(readlink -m "$link")
  case $target in
    "$install_root"/*|"$data_home/omadrop-screensaver"/*) unlink "$link" ;;
  esac
}

remove_bindings() {
  local bindings=$config_home/hypr/bindings.lua
  [[ -f $bindings ]] || return 0
  grep -q '^-- omadrop:bindings:start$' "$bindings" || return 0
  local backup temporary
  backup=$bindings.bak.$(date +%s)
  cp -p "$bindings" "$backup"
  temporary=$(mktemp "$(dirname "$bindings")/.omadrop-bindings.XXXXXX")
  awk '
    /^-- omadrop:bindings:start$/ { managed = 1; next }
    /^-- omadrop:bindings:end$/ { managed = 0; next }
    !managed { print }
  ' "$bindings" > "$temporary"
  chmod --reference="$bindings" "$temporary"
  mv "$temporary" "$bindings"
  if command -v hyprctl >/dev/null && [[ ${OMADROP_SKIP_HYPR_RELOAD:-0} != 1 ]]; then
    hyprctl reload >/dev/null
    local errors
    errors=$(hyprctl configerrors 2>&1 || true)
    if [[ -n $errors ]]; then
      cp -p "$backup" "$bindings"
      hyprctl reload >/dev/null || true
      echo "uninstall: Hyprland rejected the binding removal; restored $backup" >&2
      echo "$errors" >&2
      exit 1
    fi
  fi
}

remove_bindings

# Older installers also put helper symlinks in PATH; drop the ones we own.
for name in ttfx-music omadrop-effects omadrop-screensaver omadrop-screensaver-run omadrop-preview omadrop-close-window omadrop-demo omadrop-demo-record omadrop-doctor; do
  remove_link "$bin_dir/$name"
done
remove_link "$bin_dir/omadrop"
if [[ ( -e $bin_dir/omadrop.before-omadrop || -L $bin_dir/omadrop.before-omadrop ) \
    && ! -e $bin_dir/omadrop && ! -L $bin_dir/omadrop ]]; then
  mv -- "$bin_dir/omadrop.before-omadrop" "$bin_dir/omadrop"
fi

rm -f -- "$data_home/applications/omadrop.desktop" "$data_home/applications/omadrop-effects.desktop"
rm -f -- "$data_home/icons/hicolor/scalable/apps/omadrop.svg"

if [[ -f $install_root/VERSION && -x $install_root/bin/omadrop ]]; then
  find "$install_root" -depth -delete
  echo "Omadrop removed."
else
  echo "uninstall: no Omadrop installation found at $install_root" >&2
fi
echo "Preserved settings: ${XDG_CONFIG_HOME:-$HOME/.config}/omadrop"
echo "Preserved cover cache: ${XDG_CACHE_HOME:-$HOME/.cache}/omadrop"
