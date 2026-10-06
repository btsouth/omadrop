#!/bin/bash
# Build and install the Omadrop Arch package in a clean archlinux container.
#
# Copies this checkout into the container, builds the package with makepkg as a
# non-root user, installs it with pacman, and smoke-checks the installed layout,
# linked libraries, commands, Qt controller and preset count. The built package
# is copied to dist/.
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
repo=$(cd "$here/.." && pwd)
image=${OMADROP_TEST_IMAGE:-archlinux:latest}
name=omadrop-pkg-test-$$
container=
dist=$repo/dist

cleanup() {
  [[ -z $container ]] || docker rm -f "$container" >/dev/null 2>&1 || true
}
trap cleanup EXIT

echo "==> pulling $image"
docker pull "$image" >/dev/null

container=$(docker run -d --name "$name" --network host "$image" sleep infinity)

docker exec "$container" bash -euc '
  pacman -Sy --noconfirm --needed base-devel git >/dev/null
  useradd -m build
'

echo "==> copying the checkout"
docker exec "$container" mkdir -p /home/build/omadrop
docker cp "$repo/." "$container:/home/build/omadrop/"
docker exec "$container" chown -R build:build /home/build/omadrop

echo "==> installing dependencies from the PKGBUILD"
docker exec "$container" bash -euc '
  cd /home/build/omadrop/packaging
  source PKGBUILD
  pacman -S --noconfirm --needed --asdeps "${depends[@]}" "${makedepends[@]}"
'

echo "==> building the package"
docker exec -u build -e HOME=/home/build -w /home/build/omadrop/packaging "$container" \
  bash -uc 'mkdir -p /home/build/.cache /home/build/.gnupg; bash ./makepkg.sh'

pkg=$(docker exec "$container" bash -c 'ls /home/build/omadrop/packaging/omadrop-*.pkg.tar.zst | head -n1')
echo "==> installing $pkg"
docker exec "$container" pacman -U --noconfirm "$pkg" >/dev/null

echo "==> smoke checks"
docker exec "$container" bash -euc '
  root=/usr/lib/omadrop
  for f in \
    bin/omadrop bin/omadrop-ui bin/omadrop-milkdrop bin/omadrop-osaka \
    bin/mpris-state bin/art-fetch bin/omadrop-doctor bin/omadrop-close-window \
    experiments/projectm-ascii/projectm-ascii-live experiments/projectm-ascii/run-collection.sh \
    presets/pilot.txt VERSION licenses/THIRD_PARTY_NOTICES.md licenses/noto-sans-cjk-OFL.txt \
    shaders/native scene-api/1 worlds/osaka-jade/scene.json worlds/osaka-jade/art.svg worlds/schema/scene-v1.schema.json; do
    [[ -e "$root/$f" ]] || { echo "missing: $root/$f" >&2; exit 1; }
  done
  for f in "$root/bin/omadrop-ui" "$root/bin/omadrop-osaka" "$root/experiments/projectm-ascii/projectm-ascii-live"; do
    libs=$(ldd "$f" | awk "/not found/ {print \$1}")
    [[ -z "$libs" ]] || { echo "missing libraries for $f: $libs" >&2; exit 1; }
  done
  [[ $(ls "$root"/lib/libprojectM-4.so* | wc -l) -ge 1 ]]
  [[ -L /usr/bin/omadrop ]]
  [[ -f /usr/share/applications/omadrop.desktop ]]
  [[ -f /usr/share/icons/hicolor/scalable/apps/omadrop.svg ]]
  [[ -n $(ls /usr/share/licenses/omadrop/) ]]
  [[ $(ls "$root"/presets/pilot/*.milk | wc -l) -eq 21 ]]
  omadrop --help >/dev/null
  # Exercise the installed path without a development override or working dir.
  unset OMADROP_WORLDS
  cd /tmp
  timeout 60s "$root/bin/omadrop-osaka" --verify-render --seed 1 --width 1920 --height 1080 --scale 1
  export QT_QPA_PLATFORM=offscreen
  "$root/bin/omadrop-ui" --controls >/tmp/omadrop-ui.log 2>&1 &
  ui=$!
  sleep 3
  if ! kill -0 "$ui" 2>/dev/null; then
    echo "omadrop-ui exited before 3 seconds" >&2
    cat /tmp/omadrop-ui.log >&2
    exit 1
  fi
  kill "$ui" 2>/dev/null || true
  wait "$ui" 2>/dev/null || true
'
echo "==> the Qt controller ran offscreen for 3 seconds"

docker exec "$container" pacman -R --noconfirm omadrop >/dev/null
docker exec "$container" bash -euc '[[ ! -e /usr/lib/omadrop ]]'

mkdir -p "$dist"
docker cp "$container:$pkg" "$dist/" >/dev/null

echo "ARCH PACKAGE OK"
