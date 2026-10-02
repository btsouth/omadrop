# Architecture

Omadrop opens the Qt controls first. The controller starts one of two backends,
hides during playback, and returns when the backend closes. It manages the
backend process group so changing modes or cancelling a launch closes its windows.

| Path | Purpose |
| --- | --- |
| `app/` | Qt Quick controls, thumbnail browser, settings, process management |
| `bin/omadrop` | Public dispatcher and controller entry point |
| `bin/omadrop-milkdrop` | Display selection and MilkDrop renderer launch |
| `experiments/projectm-ascii/` | C++ renderer, PipeWire capture, compositor and tests |
| `experiments/milkdrop-audio-pilot/` | projectM patch recipe and 21 preset adaptations |
| `screensaver/ttfx/` | Music-driven ttfx fork used by Omarchy mode |
| `packaging/` | Arch package, desktop entry, icon and container checks |
| `presets/` | Original collection, provenance and texture assets |
| `tests/` | Product, installation and collection regressions |

The historical `experiments/` names are structural paths used by the build and
runtime. `bin/build-install-runtime` builds the patched projectM renderer and
installed helper tools. `install.sh` also builds ttfx and the controller, then
stages one runtime root. The package puts that root in `/usr/lib/omadrop`; a
user install uses `~/.local/share/omadrop`.

MilkDrop launches through `experiments/projectm-ascii/run-collection.sh`.
Omarchy launches through `bin/omadrop-screensaver` and
`bin/omadrop-screensaver-run`, running `ttfx-music` inside fullscreen terminals.
Both capture the default PipeWire output locally. MPRIS helpers retrieve album
art for MilkDrop. A selected effect can be previewed independently of rotation.

## Settings

Settings live under `${XDG_CONFIG_HOME:-~/.config}/omadrop`:

| File | Contents |
| --- | --- |
| `product.conf` | Controller choices, including mode, display and ASCII |
| `mode.conf` | Dispatcher mode selection |
| `preferences.conf` | Renderer preferences and shared display choice |
| `scenes.conf` | Hidden MilkDrop scenes |
| `effects.conf` | Favorite and hidden Omarchy effects |
| `sync-by-sink/*.ms` | Audio timing for each output device |
| `sync-ms`, `ascii-enabled` | Legacy settings read for migration |

The effect helper locks and atomically merges preference edits. User installation
and uninstallation preserve settings. Audio timing is output-specific; real
Bluetooth timing and multi-monitor behavior still need hardware issue reports.

## Retained compatibility paths

`shaders/native/` is loaded by the renderer and checked by the GPU probe.
`scene-api/` and scene-pack tools are still built and installed. Their test
example remains in `examples/scene-pack/`. Older presets remain inputs to
compatibility launchers and tests. These are retained dependencies, rather than
additional modes advertised by the controls.

`OMADROP_LEGACY=1` in the protected MilkDrop launcher still references
`shell.qml`. Its root `build.sh`, QML shaders and spectrum helpers are retained
for that checkout-only recovery path. The normal 0.5 install uses the C++ player.
