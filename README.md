# Omadrop

Music visuals for Omarchy: 21 MilkDrop scenes and 37 terminal effects in one app.

![Omadrop controls and thumbnail browser](docs/media/controls.png)

## What it does

Open Omadrop to a compact control panel with a thumbnail grid. Pick a mode,
choose a card, or press Play.

- MilkDrop: 21 curated presets with added audio response,
  shuffled rotation with blended transitions, and an optional ASCII dot filter.
- Omarchy (early): the 37 ttfx effects from the Omarchy screensaver, driven by your
  music. A bigger upgrade to this mode is planned for the next release.

Audio is captured locally from PipeWire. Browse scenes and effects, hide ones
you want to skip, and favorite Omarchy effects. MilkDrop cards include ASCII
previews. Esc returns from playback to the controls.

## Install

Download the Arch package from the [release page](https://github.com/btsouth/omadrop/releases), then:

```sh
sudo pacman -U omadrop-*.pkg.tar.zst
```

To build an Arch package from source:

```sh
packaging/makepkg.sh -si
```

Or build and install for your user:

```sh
./install.sh
```

The package installs to `/usr/lib/omadrop`. The user installer builds the same
app under `~/.local/share/omadrop`, with a command in `~/.local/bin` and a desktop
entry. Close Omadrop before updating. See [building](docs/building.md) for
options and checks. Remove a user installation with `./uninstall.sh`.

## Use

Open Omadrop from the launcher or run `omadrop`. Select MilkDrop or Omarchy,
then press Play or click a card. Esc returns to the controls so you can choose
another scene, effect, or mode.

| Key | MilkDrop | Omarchy |
| --- | --- | --- |
| Esc | Return to controls | Return to controls |
| N / P | Next / previous scene | Next / previous effect |
| A | Toggle ASCII | No equivalent |
| O | Toggle added audio response | No equivalent |
| [ / ] | Adjust audio timing by 10 ms | Uses saved timing |
| F11 | Toggle fullscreen | Fullscreen terminal |

See [controls](docs/controls.md) for the full key list and
command-line options. Multi-monitor playback and Bluetooth output timing are
supported; please [report issues](https://github.com/btsouth/omadrop/issues).

## Requirements

Omarchy on Arch Linux with Hyprland, PipeWire audio, an OpenGL 3.3 capable GPU,
Qt 6, and a supported terminal for Omarchy mode (Ghostty by default). Album art
needs a player that exposes MPRIS artwork. The package installs its dependencies;
the source installer can install build dependencies through Arch's package manager.

## Credits and licensing

Omadrop code is [MIT licensed](LICENSE). MilkDrop presets keep their authors'
rights, including work by Geiss, Martin, Flexi, and their collaborators. projectM,
projectm-eval, hlslparser, the texture pack, ttfx, TerminalTextEffects, Qt, and
media credits are listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

Rights holders can request a correction or removal through
[GitHub issues](https://github.com/btsouth/omadrop/issues/new/choose).

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for development checks and bug reports.
