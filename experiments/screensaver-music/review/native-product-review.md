# Omadrop native product candidate — 2026-10-02

The user rejected the dispatcher/Zenity experience as disjointed and explicitly
chose immediate visuals with controls on demand. The installed candidate now has
one Omadrop desktop entry and a Qt Quick controller. Opening the app starts the
remembered available mode. Reopening reaches the same instance and returns to
controls. Escape exits MilkDrop; any key exits Omarchy. The controls provide both
modes, shared display choice, MilkDrop ASCII, effect search/filter, favorites,
hiding and single-effect previews. Actual rendering still uses the established
separate fullscreen backend windows; this is not embedded rendering.

## Evidence

- Native app compiled on devbox with Qt 6.4.2; 27 QCoreApplication controller
  regressions passed, with no teardown warnings in the final run. They cover
  cancellation, queued launches, malformed/hanging compositor queries, startup
  deadlines, runtime failures, concurrent preference refresh and write failures.
- Dispatcher mock integration and effects preference regressions passed locally.
- Bundle checks passed fresh and repeated installs, existing-file restoration,
  preservation of settings/newer commands, and uninstall.
- Real GUI interactions in isolated omabox desktops (Omarchy
  4.0.0.r6694.g821ae58-1, Osaka Jade, 1920x1080, scale 1): cold Omarchy launch,
  return to controls, repeated-launch singleton, favorite/hide persistence,
  preview of a hidden effect, MilkDrop launch/return, ASCII launch/return, and
  clean controller quit. Fresh-account bundle installation automatically chose
  Omarchy without MilkDrop; return to controls and uninstall were checked.
- The accepted MilkDrop renderer and ff80 ttfx-music engine were not changed.

## Installation and artifacts

Installed command: ~/.local/bin/omadrop. Native controller:
~/.local/share/omadrop-product/bin/omadrop-ui. Previous product and screensaver
folders are retained as *.before-native-20261002 under ~/.local/share. The old
Modes and Effects entries were retired; no settings were overwritten.

The archive beside this report is a local Omarchy x86_64 preview bundle with
install/uninstall scripts, native controller, 37 effects, and MIT attribution.
It includes no private music, MilkDrop presets or textures. Existing MilkDrop
is optional; the full two-collection public distribution remains unfinished
until redistribution terms are resolved. An Arch PKGBUILD is provided in source,
but makepkg/pacman installation was not tested. No public upload or commit made.

The GUI box has no real audio devices or physical monitors. New Bluetooth sync,
hardware output latency, multi-monitor behavior and aesthetic acceptance are not
claimed. This changes the product flow, not the accepted audio tuning or scenes.
