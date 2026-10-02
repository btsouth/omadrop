#!/bin/bash
set -euo pipefail
cd "$(dirname "$(readlink -f "$0")")"
# Arch's projectM-4.pc currently emits -l:projectM-4, but the installed shared
# object follows the normal libprojectM-4.so naming convention.
g++ -std=c++20 -O2 -Wall -Wextra main.cpp -o projectm-ascii \
  $(pkg-config --cflags projectM-4 sdl2) \
  $(pkg-config --libs sdl2) -lprojectM-4 -lGL

"../../bin/build-milkdrop-runtime"

g++ -std=c++20 -O2 -Wall -Wextra live_settings_test.cpp live_settings.cpp \
  -o live-settings-test

g++ -std=c++20 -O2 -Wall -Wextra audio_sink_query_test.cpp live_settings.cpp \
  -o audio-sink-query-test

g++ -std=c++20 -O2 -Wall -Wextra first_run_controls_test.cpp \
  -o first-run-controls-test

g++ -std=c++20 -O2 -Wall -Wextra audio_output_session_test.cpp \
  audio_output_session.cpp pipewire_capture.cpp -o audio-output-session-test

g++ -std=c++20 -O2 -Wall -Wextra mpris_state_test.cpp \
  -o mpris-state-test $(pkg-config --cflags --libs json-c)

g++ -std=c++20 -O2 -Wall -Wextra mpris_poller_test.cpp mpris_poller.cpp \
  -o mpris-poller-test $(pkg-config --cflags --libs json-c)

g++ -std=c++20 -O2 -Wall -Wextra track_session_test.cpp track_session.cpp \
  display_session.cpp cover_presentation.cpp mpris_poller.cpp -o track-session-test $(pkg-config --cflags --libs json-c)

g++ -std=c++20 -O2 -Wall -Wextra display_session_test.cpp display_session.cpp \
  -o display-session-test

g++ -std=c++20 -O2 -Wall -Wextra scripted_scene_sequence_test.cpp \
  scripted_scene_sequence.cpp -o scripted-scene-sequence-test

g++ -std=c++20 -O2 -Wall -Wextra demo_sequence_test.cpp \
  -o demo-sequence-test

g++ -std=c++20 -O2 -Wall -Wextra paired_transport_test.cpp \
  paired_transport.cpp -o paired-transport-test

g++ -std=c++20 -O2 -Wall -Wextra paired_sync_test.cpp \
  -o paired-sync-test

g++ -std=c++20 -O2 -Wall -Wextra session_lifecycle_test.cpp \
  -o session-lifecycle-test

g++ -std=c++20 -O2 -Wall -Wextra audio_features_test.cpp \
  -o audio-features-test $(pkg-config --cflags --libs fftw3f)

g++ -std=c++20 -O2 -Wall -Wextra preset_profiles_test.cpp \
  -o preset-profiles-test

g++ -std=c++20 -O2 -Wall -Wextra preset_selector_test.cpp \
  -o preset-selector-test

g++ -std=c++20 -O2 -Wall -Wextra preset_adapters_test.cpp \
  -o preset-adapters-test

g++ -std=c++20 -O2 -Wall -Wextra audio_queue_test.cpp \
  -o audio-queue-test

g++ -std=c++20 -O2 -Wall -Wextra structure_timeline_test.cpp \
  -o structure-timeline-test $(pkg-config --cflags --libs json-c)

g++ -std=c++20 -O2 -Wall -Wextra musical_structure_test.cpp \
  -o musical-structure-test

g++ -std=c++20 -O2 -Wall -Wextra arrangement_classifier_test.cpp \
  -o arrangement-classifier-test

g++ -std=c++20 -O2 -Wall -Wextra music_frame_test.cpp \
  -o music-frame-test $(pkg-config --cflags --libs fftw3f)

g++ -std=c++20 -O2 -Wall -Wextra musical_motion_test.cpp \
  -o musical-motion-test $(pkg-config --cflags --libs fftw3f)

g++ -std=c++20 -O2 -Wall -Wextra signal_monitor_test.cpp \
  -o signal-monitor-test $(pkg-config --cflags --libs fftw3f)

g++ -std=c++20 -O2 -Wall -Wextra live_compositor_test.cpp live_compositor.cpp live_projectm.cpp \
  -o live-compositor-test $(pkg-config --cflags --libs sdl2 glew) -lprojectM-4 -lGL

g++ -std=c++20 -O2 -Wall -Wextra status_overlay_test.cpp status_overlay.cpp \
  -o status-overlay-test $(pkg-config --cflags --libs sdl2 glew) -lGL

g++ -std=c++20 -O2 -Wall -Wextra gpu_pass_timer_test.cpp \
  -o gpu-pass-timer-test $(pkg-config --cflags --libs sdl2 glew) -lGL

g++ -std=c++20 -O2 -Wall -Wextra adaptive_render_quality_test.cpp \
  -o adaptive-render-quality-test

g++ -std=c++20 -O2 -Wall -Wextra cover_presentation_test.cpp \
  cover_presentation.cpp -o cover-presentation-test

g++ -std=c++20 -O2 -Wall -Wextra native_scene_state_test.cpp \
  -o native-scene-state-test $(pkg-config --cflags --libs fftw3f)

g++ -std=c++20 -O2 -Wall -Wextra native_scene_list.cpp \
  -o native-scene-list

g++ -std=c++20 -O2 -Wall -Wextra native_renderer_test.cpp native_renderer.cpp \
  -o native-renderer-test $(pkg-config --cflags --libs sdl2 glew fftw3f) -lGL

g++ -std=c++20 -O2 -Wall -Wextra native_music_connection_test.cpp native_renderer.cpp \
  -o native-music-connection-test $(pkg-config --cflags --libs sdl2 glew fftw3f) -lGL

g++ -std=c++20 -O2 -Wall -Wextra gpu_probe.cpp native_renderer.cpp \
  -o gpu-probe $(pkg-config --cflags --libs sdl2 glew fftw3f) -lGL

g++ -std=c++20 -O2 -Wall -Wextra scene_pack_audit.cpp native_renderer.cpp \
  -o scene-pack-audit $(pkg-config --cflags --libs sdl2 glew fftw3f) -lGL

g++ -std=c++20 -O2 -Wall -Wextra scene_pack_author.cpp native_renderer.cpp \
  -o scene-pack-author $(pkg-config --cflags --libs sdl2 glew fftw3f) -lGL

g++ -std=c++20 -O2 -Wall -Wextra native_renderer_soak.cpp native_renderer.cpp \
  -o native-renderer-soak $(pkg-config --cflags --libs sdl2 glew fftw3f) -lGL

g++ -std=c++20 -O2 -Wall -Wextra native_transition_test.cpp \
  native_renderer.cpp live_compositor.cpp -o native-transition-test \
  $(pkg-config --cflags --libs sdl2 glew fftw3f) -lGL

g++ -std=c++20 -O2 -Wall -Wextra native_song_replay.cpp native_renderer.cpp live_compositor.cpp \
  -o native-song-replay $(pkg-config --cflags --libs sdl2 glew fftw3f) -lGL

g++ -std=c++20 -O2 -Wall -Wextra scorecard_fixture.cpp \
  -o scorecard-fixture

g++ -std=c++20 -O2 -Wall -Wextra audio_match.cpp \
  -o audio-match

g++ -std=c++20 -O2 -Wall -Wextra audio_replay.cpp \
  -o audio-feature-replay $(pkg-config --cflags --libs fftw3f)

g++ -std=c++20 -O2 -Wall -Wextra musical_voice_test.cpp \
  -o musical-voice-test $(pkg-config --cflags --libs fftw3f)

g++ -std=c++20 -O2 -Wall -Wextra collection_playlist_test.cpp \
  -o collection-playlist-test $(pkg-config --cflags --libs fftw3f)

g++ -std=c++20 -O2 -Wall -Wextra collection_render_test.cpp native_renderer.cpp live_compositor.cpp \
  -o collection-render-test $(pkg-config --cflags --libs sdl2 glew fftw3f) -lGL

g++ -std=c++20 -O2 -Wall -Wextra collection_audio_test.cpp native_renderer.cpp \
  -o collection-audio-test $(pkg-config --cflags --libs sdl2 glew fftw3f) -lGL

g++ -std=c++20 -O2 -Wall -Wextra scene_caption_test.cpp status_overlay.cpp \
  -o scene-caption-test $(pkg-config --cflags --libs glew) -lGL
