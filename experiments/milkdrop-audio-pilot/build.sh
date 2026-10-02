#!/bin/bash
set -euo pipefail
root=$(cd "$(dirname "$0")/../.." && pwd)
cd "$root"
"$root/bin/fetch-projectm"
python3 experiments/milkdrop-audio-pilot/prepare.py
pilot="$root/cache/milkdrop-audio-pilot"
cmake -S "$pilot/projectm" -B "$pilot/build" -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_SYSTEM_PROJECTM_EVAL=OFF -DENABLE_PLAYLIST=OFF -DCMAKE_INSTALL_PREFIX="$pilot/runtime" > "$pilot/configure.log" 2>&1
cmake --build "$pilot/build" -j4 > "$pilot/build.log" 2>&1
cmake --install "$pilot/build" > "$pilot/install.log" 2>&1
mkdir -p "$pilot/runtime/experiments/projectm-ascii"
cd "$root/experiments/projectm-ascii"
g++ -std=c++20 -O2 -Wall -Wextra "$pilot/live.cpp" -I"$pilot/runtime/include" -I. -I../milkdrop-audio-pilot \
 audio_output_session.cpp cover_presentation.cpp display_session.cpp live_assets.cpp \
 live_compositor.cpp live_projectm.cpp live_settings.cpp mpris_poller.cpp track_session.cpp \
 native_renderer.cpp paired_transport.cpp pipewire_capture.cpp scripted_scene_sequence.cpp status_overlay.cpp \
 -o "$pilot/runtime/experiments/projectm-ascii/projectm-audio-pilot.tmp" \
 $(pkg-config --cflags --libs sdl2 glew libpng fftw3f json-c) -L"$pilot/runtime/lib" -lprojectM-4 -lGL -Wl,-rpath,'$ORIGIN/../../lib'
mv "$pilot/runtime/experiments/projectm-ascii/projectm-audio-pilot.tmp" "$pilot/runtime/experiments/projectm-ascii/projectm-audio-pilot"
mkdir -p "$pilot/runtime/presets" "$pilot/runtime/bin"
cp -a "$root/presets/textures" "$pilot/runtime/presets/"
cp -a "$root/presets/milkdrop-originals" "$pilot/runtime/presets/"
cp "$root/presets/milkdrop-originals/playlist.txt" "$pilot/runtime/presets/rotation.txt"
cp -a "$root/bin/mpris-state" "$root/bin/art-fetch" "$pilot/runtime/bin/"
mkdir -p "$pilot/runtime/presets/pilot"
cp -a "$pilot/presets/." "$pilot/runtime/presets/pilot/"
printf 'Built %s\n' "$pilot/runtime/experiments/projectm-ascii/projectm-audio-pilot"
python3 -c 'import json, pathlib; p=pathlib.Path("../../experiments/milkdrop-audio-pilot/manifest.json"); pathlib.Path("../../cache/milkdrop-audio-pilot/runtime/presets/pilot.txt").write_text("".join(x["preset"]+"\n" for x in json.loads(p.read_text())["presets"]))'
