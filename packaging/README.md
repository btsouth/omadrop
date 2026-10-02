# Omadrop

Turn your music into motion. Open Omadrop and the visuals start immediately.
Play music in any app; Omadrop follows the system audio.

Leave playback to open the controls. Choose a mode, adjust the display, or
browse, preview, favorite and hide effects. Press Play to return to the visuals.

## Controls

- **Esc** returns from MilkDrop to controls. In Omarchy mode, **any key** returns.
- Opening **Omadrop again** brings up controls for the running session.
- **Space** in controls starts playback. **Q** in controls quits. **?** shows help.
- MilkDrop: **N/P** changes scene, **F11** changes fullscreen, **O** toggles the
  added music response. The ASCII option is in the controls.

## Install this preview

This candidate is for **Omarchy on Arch Linux, x86_64**. It uses the system's Qt6
Quick Controls, Ghostty, Python3, PipeWire tools, Hyprland, jq and socat.

Extract the archive and run `./install.sh`. Then open **Omadrop** from your app
launcher. Run `./uninstall.sh` from the extracted folder to remove this preview.
Your music apps and Omadrop settings are preserved. Existing runtime files are
backed up when replaced; uninstall restores them if they have not been edited.

## Included modes

**Omarchy** includes all 37 actual ttfx effects, driven by music. Every enabled
effect gets one turn per round. Favorites receive priority; hiding is reversible.

**MilkDrop** is available when the established Omadrop MilkDrop runtime is already
installed on the account. This candidate does not bundle that preset collection
or its textures. It is an Omarchy preview with optional existing MilkDrop support,
not a complete public release of both collections.

## Credits

Omadrop code is MIT licensed; see the included LICENSE. The effects originate
in TerminalTextEffects by ChrisBuilds. ttfx ports those authored effects;
its license and attribution are included separately. Qt is provided by the
system. No private music recordings are included.
