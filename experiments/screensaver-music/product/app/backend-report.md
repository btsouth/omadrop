# Omadrop native product controller — implementation report

## What this is

One persistent native Qt6 application around the preserved renderers. It owns
the lifecycle: start visuals immediately by default, drive the legacy
`omadrop` dispatcher, learn from Hyprland when the session has mapped or gone
away, and expose a small QML-facing state model. The visuals, effects and
renderer binaries are untouched.

Target: `omadrop-ui`. The QML root `src/Main.qml` (window, controls) is owned
by another worker; this app supplies `backend` and `theme` as context
properties and does not create the window itself.

## Files

- `omadrop-ui.pro` — qmake project, C++17, Qt6 (core gui qml quick
  quickcontrols2 network).
- `src/resources.qrc` — embeds `Main.qml` at `qrc:/Main.qml`.
- `src/main.cpp` — QGuiApplication, `DesktopFileName=omadrop`, per-user
  singleton `QLocalServer` (`omadrop-<uid>`, user-only socket), argument
  handling, context properties, `aboutToQuit` teardown.
- `src/backend.h`, `src/backend.cpp` — the controller.
- `src/theme.h`, `src/theme.cpp` — live Omarchy accent.
- `tests/tests.pro`, `tests/tst_backend.cpp`, `tests/run-tests.sh` — no-GUI
  unit/smoke tests with fake dispatcher, hyprctl and effects processes.

## Frontend interface (frozen)

`Q_PROPERTY`: `QString mode, display, error, status`; `bool ascii, playing,
busy, milkdropAvailable, omarchyAvailable`; `QVariantList effects`.
`Q_INVOKABLE`: `setMode, setDisplay, setAscii, play, preview, stop,
toggleFavorite, toggleHidden, clearError`.
Signals: `showControls(), stateChanged(), effectsChanged()`.
Effects entries: `{slug, name, description, favorite, hidden}`.
Theme: `accent` (QString), live, fallback `#a7d8cf`.

## Behaviour

- **Paths** — the controller is the executable's sibling `omadrop`, overridable
  with `OMADROP_CONTROLLER_BACKEND`; the effects helper is the sibling
  `omadrop-effects` (`OMADROP_EFFECTS_HELPER`); the effects binary defaults to
  `$XDG_DATA_HOME/omadrop-screensaver/bin/ttfx-music`
  (`OMADROP_EFFECTS_BINARY`). The dispatcher is always called with explicit
  flags (`--mode …`, `--preview-effect …`, `--stop`) — never bare and never
  recursively.
- **Launch** — `play()` runs `omadrop --mode <mode> --single|--all` plus
  `--ascii|--no-ascii` for MilkDrop, asynchronously, and marks busy.
- **Detection** — polls `hyprctl clients -j` every 300 ms for up to 12 s. Ours
  is the `org.omadrop.screensaver` class; MilkDrop is matched by the window
  PID's exact `/proc/<pid>/exe` (`…/projectm-ascii/projectm-ascii-live`), with
  a `title == "Omadrop"` fallback that excludes this app's own `omadrop` app
  id. On map: `playing=true, busy=false`. When every session window is gone:
  `playing=false` and `showControls()`.
- **Failures** — a launcher exit 0 before a window maps is *not* completion; a
  hard failure or a startup timeout sets a readable `error`, clears busy and
  emits `showControls()` so the window reappears. Timeout kills the polling
  query only, never the renderer.
- **Preferences** — mode/display/ascii load from the controller's own
  `$XDG_CONFIG_HOME/omadrop/product.conf`, falling back to the renderer's
  `mode.conf` / `preferences.conf` on first run (ascii defaults to false when
  unset). Selections are saved to `product.conf` with `QSaveFile`; the
  renderer's files are never rewritten.
- **Effects** — discovered with `omadrop-effects --list`; `--favorite` /
  `--hide` toggles go through the helper, which preserves the locked shared
  preferences. `effects.conf` is never edited directly.
- **Singleton** — a second launch connects and asks the primary to stop its
  renderer and show controls; `--quit` stops the session and quits. `--controls`
  starts with no renderer. `aboutToQuit` runs `--stop` and waits up to 3 s so no
  renderer is orphaned.

## Verification

- `tests/run-tests.sh` (or `qmake6 tests/tests.pro && make && ./backend-tests`):
  11 passed, 0 failed — defaults, stored-preference load, save-to-product.conf
  only, map/disappear lifecycle, startup-failure/timeout, preview, stop,
  missing controller, effects discovery + toggles.
- `g++ -fsyntax-only` against Qt 6.11.2 for `main.cpp` and `theme.cpp`.
  The app target itself needs `src/Main.qml` (other worker) before `rcc` can
  build `resources.qrc`; that build is the parent's devbox step.
