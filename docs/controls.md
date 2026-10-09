# Controls

Run `omadrop` or open Omadrop from the launcher. The controls show MilkDrop and
Omarchy, display selection, thumbnails, and the MilkDrop ASCII option. Press
Play for a MilkDrop rotation or click a card to start at that scene.
Hide scenes to exclude them from rotation. The Omarchy tab lists every installed
world; click one to play it, or press Play for the selected one. The world named
after your Omarchy theme is selected by default when it is installed, otherwise
the last world you played. A world plays until you press Esc.

## During playback

| Key | MilkDrop | Omarchy |
| --- | --- | --- |
| Esc | Return to controls | Return to controls |
| N / P, Right / Left | Next / previous scene (N / P) | No equivalent |
| A | Toggle ASCII dot filter | No equivalent |
| O | Toggle Omadrop's added audio response | No equivalent |
| [ / ] | Move saved audio timing earlier / later by 10 ms | No equivalent |
| F11 | Toggle fullscreen | Always fullscreen |
| Q | No playback shortcut | No equivalent |

In Omarchy mode Esc closes all Osaka windows and returns to the controls.

In the controls, `/` focuses search. Esc closes the controls; Q also closes them
when you are not typing. The user installer adds Super + Shift + V to open
Omadrop and Super + Alt + V to toggle a secondary display when the shortcuts
are available. Package installs do not add Hyprland shortcuts.

## Commands

```sh
omadrop --controls               # open controls
omadrop --mode milkdrop          # start MilkDrop directly
omadrop --mode omarchy           # start Omarchy directly
omadrop --scene 6                # start MilkDrop at scene 6 (1..21)
omadrop --single                 # remember one display
omadrop --all                    # remember all displays
omadrop --stop                   # stop playback
omadrop calibrate                # adjust timing for the current audio output
```

Supported MilkDrop controls are shared across displays. Preset randomness can
produce different visual details on each display. Bluetooth timing and multiple
displays are supported; report issues with your output and display setup.
