cmake_minimum_required(VERSION 3.16)
project(osaka_streaming_tests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFTW REQUIRED IMPORTED_TARGET fftw3f)
add_executable(osaka-streaming-tests streaming_tests.cpp ../src/audio.cpp ../src/score.cpp ../src/schedule.cpp)
target_link_libraries(osaka-streaming-tests PRIVATE PkgConfig::FFTW)
enable_testing()
add_test(NAME causal-audio-and-recurring-osaka COMMAND osaka-streaming-tests)
add_executable(osaka-canvas-tests canvas_tests.cpp ../src/canvas.cpp)
target_include_directories(osaka-canvas-tests PRIVATE ../src)
add_test(NAME accepted-canvas-geometry COMMAND osaka-canvas-tests)
find_package(Qt6 REQUIRED COMPONENTS Gui OpenGL)
add_executable(osaka-gpu-tests gpu_tests.cpp ../src/gpu.cpp ../src/canvas.cpp ../src/headless.cpp)
target_include_directories(osaka-gpu-tests PRIVATE ../src)
target_link_libraries(osaka-gpu-tests PRIVATE Qt6::Gui Qt6::OpenGL EGL)
add_test(NAME consecutive-effect-fidelity COMMAND osaka-gpu-tests)
add_executable(osaka-resolution-tests resolution_tests.cpp)
target_include_directories(osaka-resolution-tests PRIVATE ../src)
add_test(NAME native-first-adaptive-policy COMMAND osaka-resolution-tests)
add_executable(osaka-render-comparison-tests render_comparison_tests.cpp)
target_include_directories(osaka-render-comparison-tests PRIVATE ../src)
add_test(NAME bounded-render-comparison COMMAND osaka-render-comparison-tests)

add_executable(osaka-world-loader-tests world_loader_tests.cpp osaka-oracle.cpp ../src/kit/world-loader.cpp ../src/kit/window-label.cpp ../src/kit/piece-label.cpp ../src/kit/svg-art.cpp ../src/kit/svg-path.cpp ../src/canvas.cpp)
target_link_libraries(osaka-world-loader-tests PRIVATE Qt6::Gui Qt6::OpenGL)
target_compile_definitions(osaka-world-loader-tests PRIVATE OSAKA_WORLD_FOLDER="${CMAKE_CURRENT_SOURCE_DIR}/../../../worlds/osaka-jade")
add_test(NAME world-folder-validation COMMAND osaka-world-loader-tests)

file(GLOB OSAKA_KIT_SOURCES CONFIGURE_DEPENDS ../src/kit/*.cpp)
list(FILTER OSAKA_KIT_SOURCES EXCLUDE REGEX "/(svg-check|world-reload)\\.cpp$")
add_executable(osaka-window-shorthand-tests window_shorthand_tests.cpp
    ${OSAKA_KIT_SOURCES}
    ../src/world.cpp ../src/canvas.cpp ../src/gpu.cpp ../src/rig.cpp ../src/osaka.cpp
    ../src/headless.cpp ../src/score.cpp ../src/schedule.cpp ../src/audio.cpp)
target_include_directories(osaka-window-shorthand-tests PRIVATE ../src)
target_link_libraries(osaka-window-shorthand-tests PRIVATE Qt6::Gui Qt6::OpenGL EGL PkgConfig::FFTW)
target_compile_definitions(osaka-window-shorthand-tests PRIVATE
    WORLDS_FOLDER="${CMAKE_CURRENT_SOURCE_DIR}/../../../worlds"
    LIT_WINDOWS_FOLDER="${CMAKE_CURRENT_SOURCE_DIR}/../../../worlds/examples/lit-windows")
add_test(NAME window-shorthand-and-render COMMAND osaka-window-shorthand-tests)

add_executable(osaka-label-piece-tests label_pieces_tests.cpp
    ${OSAKA_KIT_SOURCES}
    ../src/world.cpp ../src/canvas.cpp ../src/gpu.cpp ../src/rig.cpp ../src/osaka.cpp
    ../src/headless.cpp ../src/score.cpp ../src/schedule.cpp ../src/audio.cpp)
target_include_directories(osaka-label-piece-tests PRIVATE ../src)
target_link_libraries(osaka-label-piece-tests PRIVATE Qt6::Gui Qt6::OpenGL EGL PkgConfig::FFTW)
target_compile_definitions(osaka-label-piece-tests PRIVATE
    NIGHT_STREET_FOLDER="${CMAKE_CURRENT_SOURCE_DIR}/../../../worlds/examples/night-street")
add_test(NAME label-pieces-and-backdrop COMMAND osaka-label-piece-tests)

add_executable(osaka-svg-tests svg_tests.cpp ../src/kit/svg-art.cpp ../src/kit/svg-path.cpp ../src/canvas.cpp ../src/gpu.cpp ../src/headless.cpp)
target_include_directories(osaka-svg-tests PRIVATE ../src)
target_link_libraries(osaka-svg-tests PRIVATE Qt6::Gui Qt6::OpenGL EGL)
target_compile_definitions(osaka-svg-tests PRIVATE SVG_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/svg-fixtures")
add_test(NAME supported-svg-import COMMAND osaka-svg-tests)
option(SVG_UPDATE_GOLDENS "Regenerate SVG reference PNGs on the named reference GPU" OFF)
if(SVG_UPDATE_GOLDENS)
    set_tests_properties(supported-svg-import PROPERTIES ENVIRONMENT "SVG_UPDATE_GOLDENS=1")
endif()

add_executable(omadrop-svg-import ../src/kit/svg-check.cpp ../src/kit/svg-art.cpp ../src/kit/svg-path.cpp ../src/canvas.cpp)
target_include_directories(omadrop-svg-import PRIVATE ../src)
target_link_libraries(omadrop-svg-import PRIVATE Qt6::Gui)
target_compile_definitions(osaka-svg-tests PRIVATE SVG_CHECK_TOOL="$<TARGET_FILE:omadrop-svg-import>")
add_dependencies(osaka-svg-tests omadrop-svg-import)

add_executable(osaka-check-analysis-tests check_analysis_tests.cpp ../src/kit/check-analysis.cpp)
target_include_directories(osaka-check-analysis-tests PRIVATE ../src)
add_test(NAME check-flash-analysis COMMAND osaka-check-analysis-tests)

add_executable(osaka-check-tests check_world_tests.cpp
    ${OSAKA_KIT_SOURCES}
    ../src/world.cpp ../src/canvas.cpp ../src/gpu.cpp ../src/rig.cpp ../src/osaka.cpp
    ../src/headless.cpp ../src/score.cpp ../src/schedule.cpp ../src/audio.cpp)
target_include_directories(osaka-check-tests PRIVATE ../src)
target_link_libraries(osaka-check-tests PRIVATE Qt6::Gui Qt6::OpenGL EGL PkgConfig::FFTW)
target_compile_definitions(osaka-check-tests PRIVATE WORLDS_FOLDER="${CMAKE_CURRENT_SOURCE_DIR}/../../../worlds")
set(CHECK_REPO_WORLDS "${CMAKE_CURRENT_SOURCE_DIR}/../../../worlds")
set(CHECK_TEST_WORLDS "${CMAKE_CURRENT_SOURCE_DIR}/worlds")
add_test(NAME check-osaka-jade-ready COMMAND osaka-check-tests ${CHECK_REPO_WORLDS} osaka-jade ready)
add_test(NAME check-lit-windows-ready COMMAND osaka-check-tests ${CHECK_REPO_WORLDS} examples/lit-windows ready)
add_test(NAME check-night-street-ready COMMAND osaka-check-tests ${CHECK_REPO_WORLDS} examples/night-street ready)
add_test(NAME check-hidden-water-fails-reacts COMMAND osaka-check-tests --dead-water ${CHECK_REPO_WORLDS})
add_test(NAME check-dead-pieces-fail-reacts COMMAND osaka-check-tests --dead-pieces ${CHECK_TEST_WORLDS})
add_test(NAME check-ignores-music-fails-reacts COMMAND osaka-check-tests ${CHECK_TEST_WORLDS} ignores-music fail:reacts)
add_test(NAME check-harsh-flashing-fails-flashing COMMAND osaka-check-tests ${CHECK_TEST_WORLDS} harsh-flashing fail:flashing strobe)
add_test(NAME check-unloadable-art-fails-loads COMMAND osaka-check-tests --broken-art ${CHECK_REPO_WORLDS})
set_tests_properties(check-hidden-water-fails-reacts check-osaka-jade-ready check-lit-windows-ready check-night-street-ready check-dead-pieces-fail-reacts check-ignores-music-fails-reacts
    check-harsh-flashing-fails-flashing check-unloadable-art-fails-loads PROPERTIES TIMEOUT 900)

add_executable(osaka-reload-tests world_reload_tests.cpp ../src/kit/world-reload.cpp ../src/kit/world-loader.cpp
    ../src/kit/window-label.cpp ../src/kit/piece-label.cpp ../src/kit/svg-art.cpp ../src/kit/svg-path.cpp ../src/canvas.cpp ../src/osaka.cpp)
set_target_properties(osaka-reload-tests PROPERTIES AUTOMOC ON)
target_include_directories(osaka-reload-tests PRIVATE ../src)
target_link_libraries(osaka-reload-tests PRIVATE Qt6::Gui Qt6::OpenGL)
target_compile_definitions(osaka-reload-tests PRIVATE LIT_WINDOWS_FOLDER="${CMAKE_CURRENT_SOURCE_DIR}/../../../worlds/examples/lit-windows")
add_test(NAME preview-world-reload COMMAND osaka-reload-tests)
set_tests_properties(preview-world-reload PROPERTIES TIMEOUT 120)

add_executable(swell-piece-tests swell_piece_tests.cpp ../src/kit/swell-lines.cpp
    ../src/canvas.cpp ../src/gpu.cpp ../src/headless.cpp ../src/score.cpp)
target_include_directories(swell-piece-tests PRIVATE ../src)
target_link_libraries(swell-piece-tests PRIVATE Qt6::Gui Qt6::OpenGL EGL)
add_test(NAME swell-lines-deterministic-and-music COMMAND swell-piece-tests)

add_executable(foam-piece-tests foam_piece_tests.cpp ../src/kit/foam-flecks.cpp ../src/kit/swell-lines.cpp
    ../src/canvas.cpp ../src/gpu.cpp ../src/headless.cpp ../src/score.cpp)
target_include_directories(foam-piece-tests PRIVATE ../src)
target_link_libraries(foam-piece-tests PRIVATE Qt6::Gui Qt6::OpenGL EGL)
add_test(NAME foam-flecks-deterministic-and-music COMMAND foam-piece-tests)

add_executable(great-wave-tests great_wave_tests.cpp ../src/kit/great-wave.cpp
    ../src/canvas.cpp ../src/gpu.cpp ../src/headless.cpp ../src/score.cpp)
target_include_directories(great-wave-tests PRIVATE ../src)
target_link_libraries(great-wave-tests PRIVATE Qt6::Gui Qt6::OpenGL EGL)
add_test(NAME great-wave-standing-bounded-and-music COMMAND great-wave-tests)

add_executable(boat-on-water-tests boat_on_water_tests.cpp ../src/kit/boat-on-water.cpp ../src/kit/swell-lines.cpp
    ../src/canvas.cpp ../src/gpu.cpp ../src/headless.cpp ../src/score.cpp ../src/rig.cpp)
target_include_directories(boat-on-water-tests PRIVATE ../src)
target_link_libraries(boat-on-water-tests PRIVATE Qt6::Gui Qt6::OpenGL EGL)
add_test(NAME boat-on-water-contact-deterministic-and-music COMMAND boat-on-water-tests)

add_executable(print-schedule-tests print_schedule_tests.cpp ../src/score.cpp)
target_include_directories(print-schedule-tests PRIVATE ../src)
add_test(NAME print-schedule-bounds-and-sustained-surge COMMAND print-schedule-tests)

add_executable(print-life-tests print_life_tests.cpp ../src/kit/print-life.cpp ../src/kit/swell-lines.cpp
    ../src/canvas.cpp ../src/gpu.cpp ../src/headless.cpp ../src/score.cpp ../src/schedule.cpp)
target_include_directories(print-life-tests PRIVATE ../src)
target_link_libraries(print-life-tests PRIVATE Qt6::Gui Qt6::OpenGL EGL)
add_test(NAME print-life-deterministic-and-music COMMAND print-life-tests)
