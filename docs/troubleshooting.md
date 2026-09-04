# Troubleshooting

Start with the complete local check:

```bash
omadrop-doctor
```

It returns a failure status when a required command, audio output, linked
library, OpenGL 3.3 context, native shader, or test frame is unavailable.
`omadrop-doctor --gpu` runs only the hidden GPU check.

## The window does not open or stays black

Run `omadrop-doctor --gpu`. A successful result ends with
`native_renderer=ok`. If Omadrop exited abnormally, inspect the private-safe
report with:

```bash
omadrop-doctor --crash-report
```

That output contains no audio, song metadata, artwork paths, output names, or
process IDs and is safe to attach to a bug report.

## The image barely moves

Pause and silence are intentionally near still. While a song is playing, check
that PipeWire sees both a default output and an active stream:

```bash
pactl get-default-sink
pactl list short sink-inputs
```

Omadrop follows a changed output automatically. If Bluetooth or another output
is still reconnecting, the on-screen retry status should clear after capture
becomes available.

## Album art is missing

Artwork requires an MPRIS player that publishes an art URL. Check the `MPRIS
players` count in `omadrop-doctor`. Missing artwork does not block startup and
does not change music analysis.

## A display is missing or misplaced

Use `omadrop --all` for every connected display and `omadrop --single` for one.
Either explicit choice persists; `omadrop --display-mode` prints the value a
bare launch will use.
Check the compositor's current output list with `hyprctl monitors`. Omadrop
replaces the complete display session after a hotplug so a disconnected window
cannot remain behind.

If the shortcuts were skipped because of a conflict, run `./install.sh` and
read the conflicting binding it prints. The installer does not overwrite an
unrelated shortcut.

## Two displays disagree

Toggle Omadrop off and on once. A clean launch replaces all pair-state files
and waits for the leader's first validated music frame before showing a
follower. If disagreement returns, include `omadrop-doctor --crash-report` and
the result of `omadrop-doctor --gpu` in the report.

## Motion is uncomfortable or too intense

Press `R` for reduced motion, `S` for flash limit, `I` for response intensity,
and `M` for ambient motion. These preferences persist. Flash limit is a
conservative visual option, not a medical certification.

## Reset local preferences

The preferences file is
`$XDG_CONFIG_HOME/omadrop/preferences.conf`, or
`~/.config/omadrop/preferences.conf` when `XDG_CONFIG_HOME` is unset. Move that
file aside while Omadrop is closed, then launch again to restore defaults.
