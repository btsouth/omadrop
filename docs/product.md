# Omadrop

Opening Omadrop starts the remembered visuals immediately. Press Escape in
MilkDrop, or any key in Omarchy, to return to the native controls. Opening
Omadrop again brings up those same controls rather than another instance.

The controls offer MilkDrop and Omarchy, display selection, the MilkDrop ASCII
option, and a searchable collection of all 37 actual Omarchy effects. Favorite,
hide, or preview an effect there. Favorites get priority; every enabled effect
still gets one turn per round. Hidden effects remain previewable.

The existing MilkDrop renderer and the accepted music-driven ttfx engine are
preserved. The controller manages their windows and returns when they close.
Changing modes closes the current renderer and starts the selected one.

## Build and install

The repo root is the product. `./install.sh` builds the pinned projectM runtime,
the MilkDrop collection renderer, ttfx-music and the Qt controller, then stages
the complete root under `~/.local/share/omadrop`. One `omadrop` command is
symlinked into `~/.local/bin`, with one desktop entry and icon. `--prebuilt DIR`
installs an already staged root, `--no-deps` and `--no-bindings` keep the
installer off the package manager and keyboard shortcuts. `./uninstall.sh`
removes it and preserves settings.

The Qt controller builds on the devbox with qmake6 and make from `app/build`; see
[app/README.md](../app/README.md). The Arch package is defined by
`packaging/PKGBUILD` and built from source with `packaging/makepkg.sh`;
`packaging/test-in-arch.sh` proves it in a clean `archlinux` container.

## Commands

```sh
omadrop                          # start visuals; repeated launch shows controls
omadrop --controls               # open controls
omadrop --effects                # open controls in Omarchy mode
omadrop --mode milkdrop          # direct renderer launch
omadrop --mode omarchy           # direct renderer launch
omadrop --mode milkdrop --scene N  # start on collection scene N (1..21)
omadrop --preview-effect beams   # preview a specific effect
omadrop --stop                   # stop renderer windows
```

The dispatcher and `omadrop-effects` keep their noninteractive commands:
`omadrop-effects --list`, `--favorite NAME` and `--hide NAME`. The native app
drives those helpers directly. Hidden MilkDrop scenes come from
`omadrop/scenes.conf`; hidden screensaver effects from `omadrop/effects.conf`.
Native mode, display and ASCII choices are saved in `omadrop/product.conf`; the
effect preference file is locked and atomically merged. All runtime paths honor
XDG directories, and every helper resolves its siblings from its own location.

## Verification

`bash tests/product/product-test` checks the dispatcher, scene selection and
installer behavior with mocks. `python3 tests/product/preferences-test.py`
checks concurrent effect preference edits. `packaging/test-in-arch.sh` builds and
smoke-tests the Arch package in Docker and prints `ARCH PACKAGE OK`. The native
controller has additional process, launch, failure and cancellation regressions
under `app/tests`. Run builds and full suites on the devbox.
