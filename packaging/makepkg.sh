#!/bin/bash
# Build the Omadrop Arch package from this checkout.
#
# makepkg needs its sources before build(). This snapshots the current working
# tree (HEAD plus any local changes) into a source archive next to the PKGBUILD,
# then runs makepkg. A normal checkout uses `git archive`; a tree without a
# usable git repository (for example a copied worktree) falls back to tar. The
# pinned projectM and projectm-eval sources are fetched by makepkg from the
# PKGBUILD source list.
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
repo=$(cd "$here/.." && pwd)
pkgver=$(sed -n 's/^pkgver=//p' "$here/PKGBUILD" | head -n 1)
[[ -n $pkgver ]] || { echo "makepkg: could not read pkgver from PKGBUILD" >&2; exit 1; }
archive=$here/omadrop-$pkgver.tar.gz
cleanup() { rm -f -- "$archive"; }
trap cleanup EXIT

if git -C "$repo" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  # Snapshot the working tree through a temporary index so local changes are
  # included without touching the real git index or HEAD.
  tmp_index=$(mktemp)
  rm -f -- "$tmp_index"
  tree=$(GIT_INDEX_FILE=$tmp_index git -C "$repo" read-tree HEAD && \
    GIT_INDEX_FILE=$tmp_index git -C "$repo" add -A && \
    GIT_INDEX_FILE=$tmp_index git -C "$repo" write-tree)
  rm -f -- "$tmp_index"
  commit=$(GIT_AUTHOR_NAME=omadrop GIT_AUTHOR_EMAIL=omadrop@localhost \
    GIT_COMMITTER_NAME=omadrop GIT_COMMITTER_EMAIL=omadrop@localhost \
    git -C "$repo" commit-tree "$tree" -m "omadrop $pkgver source")
  git -C "$repo" archive --format=tar.gz --prefix="omadrop-$pkgver/" -o "$archive" "$commit"
else
  tar --exclude='./.git' --exclude='./cache' --exclude='./dist' \
    --exclude='./lib' --exclude='./presets/pilot' --exclude='./presets/pilot.txt' \
    --exclude='./screensaver/ttfx/target' --exclude='./app/build' \
    --exclude='./site/node_modules' --exclude='./site/dist' --exclude='./site/.astro' \
    --exclude='./.github' --exclude='./packaging/omadrop-*.tar.gz' \
    --exclude='./packaging/*.pkg.tar.*' --exclude='./packaging/src' --exclude='./packaging/pkg' \
    -czf "$archive" -C "$repo" --transform="s,^\./,omadrop-$pkgver/," .
fi

cd "$here"
makepkg -f "$@"
