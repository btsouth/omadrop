#!/bin/bash
# Install the unified Omadrop product launcher.
#
# Stages the wrapper under XDG_DATA_HOME/omadrop-product, points the normal
# `omadrop` command at it, and adds an "Omadrop Modes" desktop entry. The
# accepted MilkDrop and screensaver backends are never overwritten.
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
data_home=${XDG_DATA_HOME:-$HOME/.local/share}
bin_dir=${OMADROP_BIN_DIR:-$HOME/.local/bin}
product_root=${OMADROP_PRODUCT_ROOT:-$data_home/omadrop-product}
milkdrop_backend=${OMADROP_MILKDROP_BACKEND:-$data_home/omadrop/bin/omadrop}
omarchy_backend=${OMADROP_OMARCHY_BACKEND:-$data_home/omadrop-screensaver/bin/omadrop-screensaver}
applications_dir=$data_home/applications

usage() {
  cat <<'EOF'
Usage: ./install.sh

Install the unified Omadrop launcher (MilkDrop + Omarchy screensaver modes).

  HOME / XDG_DATA_HOME / XDG_CONFIG_HOME select the install target.
EOF
}

while (($#)); do
  case $1 in
    -h|--help) usage; exit 0 ;;
    *) echo "install: unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

[[ $data_home == /* && $bin_dir == /* && $product_root == /* ]] || {
  echo "install: HOME and XDG paths must be absolute" >&2
  exit 1
}
[[ -x $milkdrop_backend ]] || {
  echo "install: MilkDrop backend missing or not executable: $milkdrop_backend" >&2
  exit 1
}
[[ -x $omarchy_backend ]] || {
  echo "install: Omarchy screensaver backend missing or not executable: $omarchy_backend" >&2
  exit 1
}

install -Dm755 "$here/bin/omadrop" "$product_root/bin/omadrop"
install -Dm755 "$here/bin/omadrop-effects" "$product_root/bin/omadrop-effects"
if [[ -x $here/app/build/omadrop-ui ]]; then
  temporary=$(mktemp "$product_root/bin/.omadrop-ui.XXXXXX")
  install -m755 "$here/app/build/omadrop-ui" "$temporary"
  mv "$temporary" "$product_root/bin/omadrop-ui"
fi

mkdir -p "$bin_dir"
backup=$bin_dir/omadrop.before-product
canonical_product_root=$(realpath -m -- "$product_root")
if [[ -L $bin_dir/omadrop && $(readlink -m "$bin_dir/omadrop") == "$canonical_product_root/"* ]]; then
  : # already ours; keep the original backup untouched
elif [[ -e $bin_dir/omadrop || -L $bin_dir/omadrop ]]; then
  if [[ ! -e $backup && ! -L $backup ]]; then
    mv -- "$bin_dir/omadrop" "$backup"
  fi
fi
ln -sfn "$product_root/bin/omadrop" "$bin_dir/omadrop"

mkdir -p "$applications_dir"
if [[ -x $product_root/bin/omadrop-ui ]]; then
  cat >"$applications_dir/omadrop.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Omadrop
Comment=Turn your music into motion
Exec="$bin_dir/omadrop"
Icon=omadrop
Terminal=false
Categories=AudioVideo;
EOF
  if [[ -f $here/package/omadrop.svg ]]; then
    install -Dm644 "$here/package/omadrop.svg" "$data_home/icons/hicolor/scalable/apps/omadrop.svg"
  fi
  # Retire only the earlier launchers owned by this product.
  for entry in omadrop-modes.desktop omadrop-effects.desktop; do
    path=$applications_dir/$entry
    if [[ -f $path ]] && grep -Fq "Exec=\"$bin_dir/omadrop\"" "$path"; then
      mv "$path" "$product_root/$entry.before-native"
    fi
  done
  echo "installed: $bin_dir/omadrop -> $product_root/bin/omadrop"
  echo "desktop entry: $applications_dir/omadrop.desktop"
  exit 0
fi
cat >"$applications_dir/omadrop-modes.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Omadrop Modes
Comment=Choose between the MilkDrop visualizer and the Omarchy screensaver
Exec="$bin_dir/omadrop" --choose
Terminal=false
Categories=AudioVideo;
EOF

cat >"$applications_dir/omadrop-effects.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Omadrop Effects
Comment=Browse, preview and favorite the music screensaver effects
Exec="$bin_dir/omadrop" --effects
Terminal=false
Categories=AudioVideo;
EOF

echo "installed: $bin_dir/omadrop -> $product_root/bin/omadrop"
echo "desktop entry: $applications_dir/omadrop-modes.desktop"
echo "desktop entry: $applications_dir/omadrop-effects.desktop"
