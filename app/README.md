# Omadrop controls

The Qt Quick app that opens when you run `omadrop`. It shows the MilkDrop and
Omarchy tabs, scene previews, display and ASCII settings, and starts playback.
`omadrop --play` starts the remembered mode directly. Esc in either mode
returns to the controls, and opening Omadrop again while visuals play stops
them and shows the controls.

The visuals run in separate fullscreen windows, not inside this one. The app starts the launcher with an explicit mode and display choice,
then follows the session through Hyprland: Osaka Jade windows by their
`org.omadrop.screensaver` class and MilkDrop by its renderer executable. When
those windows close, the controls come back. Startup and compositor queries are
bounded, and stopping cancels the launcher's whole process group.

Paths default to siblings of the installed `omadrop-ui` binary. For source-tree
runs, `OMADROP_CONTROLLER_BACKEND`, `OMADROP_OMARCHY_BACKEND`,
`OMADROP_MILKDROP_LIVE` and `OMADROP_COLLECTION_MANIFEST` override them.

Build with `qmake6 ../omadrop-ui.pro && make` from `app/build`.
`bash tests/run-tests.sh` builds and runs the controller tests offscreen with
fake processes; they need no desktop.
