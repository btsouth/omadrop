#!/bin/bash
# Install the music screensaver as its own command, omadrop-screensaver.
# Leaves Omarchy's screensaver, the omadrop MilkDrop app and every other
# omadrop launcher untouched.
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
dest=${XDG_DATA_HOME:-$HOME/.local/share}/omadrop-screensaver
mkdir -p "$dest/bin" "$HOME/.local/bin"
temporary=$(mktemp "$dest/bin/.ttfx-music.XXXXXX")
trap 'rm -f -- "$temporary"' EXIT
install -m 755 "$here/build/ttfx-music" "$temporary"
mv -f -- "$temporary" "$dest/bin/ttfx-music"
install -m 755 "$here/bin/omadrop-screensaver" "$here/bin/omadrop-screensaver-run" "$dest/bin/"
install -m 644 "$here/ttfx/LICENSE" "$here/ttfx/NOTICE" "$dest/"
ln -sfn "$dest/bin/omadrop-screensaver" "$HOME/.local/bin/omadrop-screensaver"
echo "installed: omadrop-screensaver ($(sha256sum "$dest/bin/ttfx-music" | cut -c1-16))"
