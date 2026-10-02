#!/bin/bash
# Build and run the controller's fake-process tests. No GUI, no real session.
# Fast enough to run anywhere Qt6 is installed; safe to run on the devbox.
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
build=${OMADROP_TEST_BUILD_DIR:-"$here/.build"}
qmake=${QMAKE:-qmake6}

mkdir -p "$build"
cd "$build"
"$qmake" "$here/tests.pro" >/dev/null
make -j"$(nproc)" >/dev/null
QT_QPA_PLATFORM=offscreen ./backend-tests
