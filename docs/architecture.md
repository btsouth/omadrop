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
| `experiments/osaka-live/` | Live Osaka Jade renderer and recurring scene schedule |
| `packaging/` | Arch package, desktop entry, icon and container checks |
| `presets/` | Original collection, provenance and texture assets |
| `tests/` | Product, installation and collection regressions |

The historical `experiments/` names are structural paths used by the build and
runtime. `bin/build-install-runtime` builds the patched projectM renderer and
installed helper tools. `install.sh` also builds Osaka and the controller, then
stages one runtime root. The package puts that root in `/usr/lib/omadrop`; a
user install uses `~/.local/share/omadrop`.

MilkDrop launches through `experiments/projectm-ascii/run-collection.sh`.
Omarchy launches the installed `bin/omadrop-osaka` binary. It creates a fullscreen
window for each selected display and quits on Escape. Its Hyprland class is
`org.omadrop.screensaver`, which the controller tracks to restore controls.
Osaka Jade is used for every theme. The dispatcher function
`omarchy_world_backend()` is the single theme-to-world choice; a future second
world changes that function and adds its binary.
Both capture the default PipeWire output locally. MPRIS helpers retrieve album
art for MilkDrop. Osaka shares the same PipeWireCapture implementation as MilkDrop: an empty
sink target records the default output monitor with stream.capture.sink=true.

## Settings

Settings live under `${XDG_CONFIG_HOME:-~/.config}/omadrop`:

| File | Contents |
| --- | --- |
| `product.conf` | Controller choices, including mode, display and ASCII |
| `mode.conf` | Dispatcher mode selection |
| `preferences.conf` | Renderer preferences and shared display choice |
| `scenes.conf` | Hidden MilkDrop scenes |
| `sync-by-sink/*.ms` | Audio timing for each output device |
| `sync-ms`, `ascii-enabled` | Legacy settings read for migration |

User installation
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
