# MilkDrop renderer

This structural directory contains Omadrop's C++ renderer and supporting tests.
The name is retained because the build and installed launchers use it.

`bin/build-milkdrop-runtime` builds patched projectM and the 21-scene player
through `experiments/milkdrop-audio-pilot/build.sh`. The player captures local
PipeWire audio, retrieves MPRIS artwork and presents continuous or ASCII output.
`run-collection.sh` is the installed MilkDrop entry point.

`bin/build-install-runtime` also builds the installed GPU, scene-pack and audio
matching tools. Native shaders and the scene API remain runtime dependencies.
`build.sh` additionally builds older renderer and unit-test tools; these source
files remain to preserve build and test coverage.

See [architecture](../../docs/architecture.md),
[building](../../docs/building.md) and [controls](../../docs/controls.md).
The product and controller checks are headless. Renderer tests that create an
OpenGL context need a suitable isolated graphics session.
