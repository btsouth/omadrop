# Native controller

The Qt Quick controls start the dispatcher with an explicit mode and display
choice. MilkDrop remains the default and keeps its scene browser and ASCII
option. Omarchy plays the installed Osaka Jade binary.

The controller tracks Osaka windows by the org.omadrop.screensaver Hyprland
class and MilkDrop by its exact renderer executable. Compositor close events
restore controls immediately. Process groups, bounded startup queries and
cancellation markers preserve session teardown.

Paths default to siblings of the installed controls binary. Source-tree
checks can override OMADROP_CONTROLLER_BACKEND, OMADROP_OMARCHY_BACKEND,
OMADROP_MILKDROP_LIVE and OMADROP_COLLECTION_MANIFEST.

Run bash app/tests/run-tests.sh on devbox for the fake-process controller
tests. They do not open a GUI or contact the real desktop.
