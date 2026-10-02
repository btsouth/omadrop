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

See [app/README.md](app/README.md) for the native Qt build and tests. After the
native binary is built, `./install.sh` installs one Omadrop desktop entry and
command over an existing two-mode installation. It backs up the previous command
and retires the old Modes and Effects launcher entries.

See [package/README.md](package/README.md) for the relocatable preview bundle and
Arch package recipe. The bundle includes Omarchy effects and can discover an
existing MilkDrop installation. MilkDrop presets, textures, and private audio
are not included.

## Commands

```sh
omadrop                          # start visuals; repeated launch shows controls
omadrop --controls               # open controls
omadrop --effects                # open controls in Omarchy mode
omadrop --app --quit             # stop visuals and quit the controller
omadrop --mode milkdrop          # direct renderer launch
omadrop --mode omarchy           # direct renderer launch
omadrop --preview-effect beams   # preview a specific effect
omadrop --stop                   # stop renderer windows
```

The dispatcher and `omadrop-effects` retain their noninteractive commands for
compatibility. The native app uses those helpers without opening Zenity dialogs.
All runtime paths honor XDG directories. Native mode, display, and ASCII choices
are saved in `omadrop/product.conf`; effect preferences are locked, atomically
merged in `omadrop/effects.conf`. No rejected native scene is offered.

## Verification

`bash tests/product-test` checks dispatcher and installer behavior with mocks.
`python3 tests/preferences-test.py` checks concurrent effect preference edits.
The native controller has additional process, launch, failure, and cancellation
regressions under `app/tests`. Run builds and full suites on the devbox. GUI
checks belong in omabox; actual audio and output latency require hardware.
