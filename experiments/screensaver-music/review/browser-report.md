# Effect browser and curation report

Scope: the shared product launcher's effect browser for the Omarchy music
screensaver. Files: `product/bin/omadrop`, `product/bin/omadrop-effects`,
`bin/omadrop-screensaver`, `product/install.sh`, `product/tests/product-test`.

## What it does

The screensaver already runs the 37 effects the installed `ttfx-music` ships.
This adds a way to browse them, try one, and decide which ones the rotation
plays:

- `omadrop --effects` opens the browser.
- `omadrop --effect NAME` launches one effect directly (and remembers Omarchy).
- `omadrop --preview-effect NAME` repeats the selected effect, on one display, and
  changes no saved choice.

## Discovery

The helper reads `ttfx-music --help` and parses the `Commands:` block into the
effect list (slug + description), then applies friendly names. It never starts
the renderer. The same `--help` parse validates every slug in the launcher
(python) and in `bin/omadrop-screensaver` (awk) before anything is stopped,
saved or opened.

## Settings

`$XDG_CONFIG_HOME/omadrop/effects.conf`, owned by the helper:

    version=1
    favorites=beams,matrix
    hidden=swarm

- atomic write (temp + rename), mode 600;
- unmanaged keys are preserved, unknown/future versions fail without a write;
- slugs are validated and de-duplicated on read and write;
- hiding the last visible effect is refused;
- a missing file means all effects enabled, no favorites.

The running rotation reloads the file on its next effect, so edits take effect
without a restart.

## Browser flow

Zenity dialogs, driven entirely by `OMADROP_ZENITY` so tests can replay them:

1. Main menu: Browse effects / Favorites / Hidden / Start rotation.
2. Effect list: one row per effect with friendly name, status and description.
3. Action for the selected effect: Preview / Favorite or Remove favorite /
   Hide or Unhide / Back.
4. Favorites and Hidden are checkbox lists (bulk edits). Start rotation launches
   a normal rotation through the product wrapper.

Cancelling any dialog changes nothing and does not touch a running session.

## Preview

The preview closes the chooser, runs `omadrop --preview-effect NAME`, then
polls `hyprctl clients -j` for the `org.omadrop.screensaver` class: it waits for
the window to appear (bounded by a startup deadline, with an error dialog if it
never does) and then for it to disappear. The stock `org.omarchy.screensaver`
class is ignored. Any key exits the preview; the remembered mode and display
choice are untouched (`OMADROP_PERSIST_DISPLAY=0`, single display).

## Tests

`bash product/tests/product-test` (sub-minute, no GUI). New coverage:

- discovery via a fake `ttfx-music --help`;
- effects.conf write, mode 600, toggle, future-version and all-hidden refusal;
- browser cancel leaves no state;
- `--effect` validation, argument propagation and mode remembering;
- `--preview-effect` writes neither mode nor display;
- screensaver launcher injects `OMADROP_EFFECT` into the compositor exec;
- browser preview waits for our window and ignores the stock class;
- installer stages the helper and the "Omadrop Effects" desktop entry.

Fixtures: `fake-ttfx`, `fake-backend` (records `OMADROP_PERSIST_DISPLAY`),
`fake-hyprctl` (client-list sequences), `fake-zenity`.
