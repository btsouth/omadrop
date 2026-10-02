# Controls

Run `omadrop` or open Omadrop from the launcher. The controls show MilkDrop and
Omarchy, display selection, thumbnails, and the MilkDrop ASCII option. Press
Play for a rotation or click a card to start at that scene or preview that effect.
Hide cards to exclude them from rotation. Omarchy favorites receive priority;
every enabled effect still gets a turn each round. A hidden effect can be previewed.

## During playback

| Key | MilkDrop | Omarchy |
| --- | --- | --- |
| Esc | Return to controls | Return to controls |
| N / P, Right / Left | Next / previous scene (N / P) | Next / previous effect |
| A | Toggle ASCII dot filter | No equivalent |
| O | Toggle Omadrop's added audio response | No equivalent |
| [ / ] | Move saved audio timing earlier / later by 10 ms | Uses saved output timing |
| F11 | Toggle fullscreen | Uses the terminal's fullscreen window |
| Q | No playback shortcut | Return to controls |

In Omarchy mode Esc and Q return to the controls, N or Right moves to the next effect, and P or Left goes back. Other keys are ignored.

In the controls, `/` focuses search. Esc closes the controls; Q also closes them
when you are not typing. The user installer adds Super + Shift + V to open
Omadrop and Super + Alt + V to toggle a secondary display when the shortcuts
are available. Package installs do not add Hyprland shortcuts.

## Commands

```sh
omadrop --controls               # open controls
omadrop --effects                # open controls in Omarchy mode
omadrop --mode milkdrop          # start MilkDrop directly
omadrop --mode omarchy           # start Omarchy directly
omadrop --scene 6                # start MilkDrop at scene 6 (1..21)
omadrop --preview-effect beams   # preview one effect
omadrop --single                 # remember one display
omadrop --all                    # remember all displays
omadrop --stop                   # stop playback
omadrop calibrate                # adjust timing for the current audio output
```

Supported MilkDrop controls are shared across displays. Preset randomness can
produce different visual details on each display. Bluetooth timing and multiple
displays are supported; report issues with your output and display setup.
