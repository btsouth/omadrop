#!/usr/bin/env python3
"""Build the Omadrop distributable bundle for Omarchy (Arch Linux, x86_64).

Stages a relocatable per-user bundle and a matching source tarball for the
local PKGBUILD. The bundle carries the native Qt GUI, the dispatcher and the
effects helper, the music screensaver, the MIT-licensed ttfx engine and the
attribution files. It never carries MilkDrop presets, textures or audio, and
it contains no private home paths.

    python3 build-bundle.py --output DIR
    python3 build-bundle.py --output DIR --gui app/build/omadrop-ui

A relative --gui is resolved against the product directory, so the default
points at product/app/build/omadrop-ui regardless of where the script runs.
"""

import argparse
import os
import shutil
import sys
import tarfile
from pathlib import Path

VERSION = "0.5.0-preview.1"
ARCHIVE = "omadrop-%s.tar.gz" % VERSION

PACKAGE_DIR = Path(__file__).resolve().parent
PRODUCT_DIR = PACKAGE_DIR.parent          # product/
PROJECT_DIR = PRODUCT_DIR.parent          # experiments/screensaver-music/
REPO_DIR = PACKAGE_DIR.parents[3]         # repository root

INSTALL_SH = r'''#!/bin/bash
# Omadrop per-user installer. Run from the unpacked bundle directory.
#
# Stages the runtime under the XDG data directories, adds one `omadrop`
# command, one Omadrop desktop entry and the Omadrop icon. A different
# existing `omadrop` command is backed up once. An existing MilkDrop or
# screensaver install and all settings are never modified.
set -euo pipefail

here=$(cd "$(dirname "$(readlink -f "$0")")" && pwd)

data_home=${XDG_DATA_HOME:-$HOME/.local/share}
state_home=${XDG_STATE_HOME:-$HOME/.local/state}
bin_dir=${OMADROP_BIN_DIR:-$HOME/.local/bin}
product_root=${OMADROP_PRODUCT_ROOT:-$data_home/omadrop-product}
screensaver_root=${OMADROP_SCREENSAVER_ROOT:-$data_home/omadrop-screensaver}
applications_dir=$data_home/applications
icons_dir=$data_home/icons/hicolor/scalable/apps
manifest=$state_home/omadrop/product-install.list
restores=$state_home/omadrop/product-restore.list
backup_dir=$state_home/omadrop/install-backups
backup=$bin_dir/omadrop.before-omadrop

while (($#)); do
  case $1 in
    -h|--help) echo "Usage: ./install.sh"; exit 0 ;;
    *) echo "install: unknown option: $1" >&2; exit 2 ;;
  esac
done

[[ $data_home == /* && $bin_dir == /* && $product_root == /* && $screensaver_root == /* ]] || {
  echo "install: HOME and XDG paths must be absolute" >&2
  exit 1
}

[[ -x $here/share/omadrop-product/bin/omadrop-ui ]] || {
  echo "install: bundle incomplete: share/omadrop-product/bin/omadrop-ui is missing" >&2
  exit 1
}

record() {
  mkdir -p "$(dirname "$manifest")"
  grep -qxF -- "$1" "$manifest" 2>/dev/null || printf '%s\n' "$1" >>"$manifest"
}

# Atomic install: write a sibling temporary file and rename it over the
# target, so a running native binary is replaced, never truncated in place.
owned() {
  [[ ! -e $1 && ! -L $1 ]] && return 0
  grep -qxF -- "$1" "$manifest" 2>/dev/null
}

stage() {
  local source=$1 target=$2 mode=$3 temporary claim=1 saved=
  owned "$target" || claim=0
  if ((claim == 0)); then
    mkdir -p "$backup_dir"
    saved=$backup_dir/$(printf '%s' "$target" | sha256sum | cut -d' ' -f1)
    if [[ ! -e $saved && ! -L $saved ]]; then cp -a -- "$target" "$saved"; fi
  fi
  mkdir -p "$(dirname "$target")"
  temporary=$(mktemp "$(dirname "$target")/.omadrop.XXXXXX")
  install -m "$mode" "$source" "$temporary"
  mv -f -- "$temporary" "$target"
  if ((claim)); then record "$target"; fi
  if [[ -n $saved ]]; then
    printf '%s\t%s\t%s\n' "$target" "$saved" "$(sha256sum "$target" | cut -d' ' -f1)" >>"$restores"
  fi
  return 0
}

for name in omadrop omadrop-effects omadrop-ui; do
  stage "$here/share/omadrop-product/bin/$name" "$product_root/bin/$name" 755
done
for name in omadrop-screensaver omadrop-screensaver-run ttfx-music; do
  stage "$here/share/omadrop-screensaver/bin/$name" "$screensaver_root/bin/$name" 755
done
for name in LICENSE ttfx-LICENSE ttfx-NOTICE; do
  stage "$here/share/licenses/omadrop/$name" "$product_root/licenses/$name" 644
done

stage "$here/share/applications/omadrop.desktop" "$applications_dir/omadrop.desktop" 644
stage "$here/share/icons/hicolor/scalable/apps/omadrop.svg" "$icons_dir/omadrop.svg" 644

# One command symlink. A prior different `omadrop` is backed up exactly once.
mkdir -p "$bin_dir"
canonical_product_root=$(realpath -m -- "$product_root")
if [[ -L $bin_dir/omadrop && $(readlink -m -- "$bin_dir/omadrop") == "$canonical_product_root/"* ]]; then
  : # already ours; keep any existing backup untouched
elif [[ -e $bin_dir/omadrop || -L $bin_dir/omadrop ]]; then
  if [[ ! -e $backup && ! -L $backup ]]; then
    mv -- "$bin_dir/omadrop" "$backup"
  fi
fi
ln -sfn "$product_root/bin/omadrop" "$bin_dir/omadrop"

echo "installed: $bin_dir/omadrop -> $product_root/bin/omadrop"
echo "desktop entry: $applications_dir/omadrop.desktop"
'''

UNINSTALL_SH = r'''#!/bin/bash
# Remove the per-user Omadrop install created by install.sh.
#
# Removes only the files that installer staged, the one `omadrop` symlink and
# the Omadrop desktop entry and icon, then restores a backed-up command. An
# existing MilkDrop install, other screensaver installs and all settings are
# never removed.
set -euo pipefail

data_home=${XDG_DATA_HOME:-$HOME/.local/share}
state_home=${XDG_STATE_HOME:-$HOME/.local/state}
bin_dir=${OMADROP_BIN_DIR:-$HOME/.local/bin}
product_root=${OMADROP_PRODUCT_ROOT:-$data_home/omadrop-product}
screensaver_root=${OMADROP_SCREENSAVER_ROOT:-$data_home/omadrop-screensaver}
applications_dir=$data_home/applications
icons_dir=$data_home/icons/hicolor/scalable/apps
manifest=$state_home/omadrop/product-install.list
restores=$state_home/omadrop/product-restore.list
backup=$bin_dir/omadrop.before-omadrop

while (($#)); do
  case $1 in
    -h|--help) echo "Usage: ./uninstall.sh"; exit 0 ;;
    *) echo "uninstall: unknown option: $1" >&2; exit 2 ;;
  esac
done

# Remove only the recorded files, and only inside the directories we manage.
if [[ -f $manifest ]]; then
  while IFS= read -r path; do
    [[ -n $path ]] || continue
    case $path in
      "$product_root"/*|"$screensaver_root"/*|"$applications_dir/omadrop.desktop"|"$icons_dir"/*)
        rm -f -- "$path" ;;
    esac
  done <"$manifest"
  rm -f -- "$manifest"
fi

# Restore pre-existing files only when the user has not changed our replacement.
if [[ -f $restores ]]; then
  while IFS=$'\t' read -r path saved digest; do
    case $path in
      "$product_root"/*|"$screensaver_root"/*|"$applications_dir/omadrop.desktop"|"$icons_dir"/*)
        if [[ -f $path && ( -e $saved || -L $saved ) ]] &&
            [[ $(sha256sum "$path" | cut -d' ' -f1) == "$digest" ]]; then
          mv -Tf -- "$saved" "$path"
        fi ;;
    esac
  done <"$restores"
fi

# Drop our directories only when nothing else (an older install) remains.
rmdir "$screensaver_root/bin" "$screensaver_root" 2>/dev/null || true
rmdir "$product_root/bin" "$product_root" 2>/dev/null || true

# The command symlink, only if it still points into our product root.
if [[ -L $bin_dir/omadrop && $(readlink -m -- "$bin_dir/omadrop") == "$(realpath -m -- "$product_root")/"* ]]; then
  rm -f -- "$bin_dir/omadrop"
fi

# Restore the prior normal command if the installer backed one up.
if [[ ( -e $backup || -L $backup ) && ! -e $bin_dir/omadrop && ! -L $bin_dir/omadrop ]]; then
  mv -- "$backup" "$bin_dir/omadrop"
fi

echo "removed: Omadrop per-user install"
'''


def fail(message):
    sys.exit("error: " + message)


def copy(source, target, mode):
    if not source.is_file():
        fail("missing required file: %s" % source)
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, target)
    os.chmod(target, mode)


def write_script(target, text):
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text, encoding="utf-8")
    os.chmod(target, 0o755)


def normalize(info):
    info.uid = info.gid = 0
    info.uname = info.gname = ""
    info.mtime = 0
    return info


def build(output, gui):
    output = Path(output).resolve()
    root = output / ("omadrop-%s" % VERSION)
    if root.exists():
        shutil.rmtree(root)

    copy(PRODUCT_DIR / "bin" / "omadrop",
         root / "share/omadrop-product/bin/omadrop", 0o755)
    copy(PRODUCT_DIR / "bin" / "omadrop-effects",
         root / "share/omadrop-product/bin/omadrop-effects", 0o755)
    copy(gui, root / "share/omadrop-product/bin/omadrop-ui", 0o755)

    copy(PROJECT_DIR / "bin" / "omadrop-screensaver",
         root / "share/omadrop-screensaver/bin/omadrop-screensaver", 0o755)
    copy(PROJECT_DIR / "bin" / "omadrop-screensaver-run",
         root / "share/omadrop-screensaver/bin/omadrop-screensaver-run", 0o755)
    copy(PROJECT_DIR / "build" / "ttfx-music",
         root / "share/omadrop-screensaver/bin/ttfx-music", 0o755)

    copy(PACKAGE_DIR / "omadrop.desktop",
         root / "share/applications/omadrop.desktop", 0o644)
    copy(PACKAGE_DIR / "omadrop.svg",
         root / "share/icons/hicolor/scalable/apps/omadrop.svg", 0o644)
    copy(PACKAGE_DIR / "README.md", root / "README.md", 0o644)

    copy(REPO_DIR / "LICENSE", root / "share/licenses/omadrop/LICENSE", 0o644)
    copy(PROJECT_DIR / "ttfx" / "LICENSE",
         root / "share/licenses/omadrop/ttfx-LICENSE", 0o644)
    copy(PROJECT_DIR / "ttfx" / "NOTICE",
         root / "share/licenses/omadrop/ttfx-NOTICE", 0o644)

    write_script(root / "install.sh", INSTALL_SH)
    write_script(root / "uninstall.sh", UNINSTALL_SH)

    archive = output / ARCHIVE
    if archive.exists():
        archive.unlink()
    with tarfile.open(archive, "w:gz") as tar:
        tar.add(root, arcname=root.name, filter=normalize)

    return root, archive


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True,
                        help="directory to write the bundle and tarball into")
    parser.add_argument("--gui", default="app/build/omadrop-ui",
                        help="native Qt GUI binary (default: app/build/omadrop-ui,"
                             " relative to the product directory)")
    args = parser.parse_args()

    gui = Path(args.gui)
    if not gui.is_absolute():
        gui = PRODUCT_DIR / gui
    gui = gui.resolve()
    if not gui.is_file():
        fail("GUI binary not found: %s\nbuild the Qt app or pass --gui PATH" % gui)

    root, archive = build(args.output, gui)
    print("bundle:  %s" % root)
    print("archive: %s" % archive)


if __name__ == "__main__":
    main()
