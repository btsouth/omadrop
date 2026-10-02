# Music rotation report (worker)

Scope: `ttfx/src/music/rotation.rs` (new), `ttfx/src/music/mod.rs` (module
export only), `ttfx/src/main.rs` (`run_music` selection + helpers),
`ttfx/src/cli.rs` (one music-only flag). No effect sources, no conductor /
tuning changes, no non-music path changes.

## What changed

- **`src/music/rotation.rs`** — reusable weighted shuffle bag.
  - Every eligible effect is offered once per round before any repeat.
  - The last `min(8, count - 1)` selections are kept out of the bag while other
    choices remain (sliding window over the whole sequence, so it also spans the
    round boundary); a one-effect collection is allowed to repeat.
  - Intensity/calm and stock duration weighting choose the order inside the
    bag; favorites get a `2.5x` share of that choice, never an extra slot.
  - `Preferences` reads `$XDG_CONFIG_HOME/omadrop/effects.conf` (else
    `$HOME/.config/omadrop/effects.conf`): `version=1`, `favorites=`, `hidden=`.
    Unknown keys/slugs and duplicates are ignored; `version>1` returns an error
    so a future file is ignored, not misread; a missing file is empty prefs.
    When every effect is hidden the last eligible set is kept (or all effects
    if there never was one) and reported — never an empty panic.
  - `apply_preferences` hides/updates mid-run: hidden effects leave the pending
    candidates, favorites re-order, and an unhidden effect may join the current
    round without repeating one that already played.
- **`src/main.rs`** — `run_music` builds the rotation from the existing stock
  weights and CALM set, filters non-effect subcommands (`help`) out of the
  candidates, reloads preferences once per effect (not per frame), and picks
  through `Rotation::next`. `--music-ignore-preferences` skips the file.
- **`src/cli.rs`** — `--music-ignore-preferences` (`requires = "music"`).

## Tests added (`src/music/rotation.rs`)

Full round coverage; boundary/recent-repeat avoidance across rounds for several
set sizes; single-effect repeat allowed; two-effect set; all-hidden fallback;
unknown/duplicate names ignored; hidden leaves pending; unhide mid-round without
repeat; favorites add no duplicate slot; favorites arrive earlier in the round;
deterministic seeded replay; preferences parse format; future version rejected;
missing file is empty.

## Checks (devbox-bts)

Source mirrored (excluding `target/`, `.git`) to
`/workspace/worktrees/omadrop-omarchy-flagship/experiments/screensaver-music/ttfx`.

```
PATH=$HOME/.cargo/bin:$PATH cargo test --locked
```
`lib`: 56 passed, 0 failed. Integration/doc tests all pass (5, 1, 1, 2, 1, 0).

```
PATH=$HOME/.cargo/bin:$PATH cargo build --release --locked
```
Finished `release` in 34s, no warnings.

Binary fetched to `experiments/screensaver-music/build/ttfx-music`.

CLI smoke (no GUI, argument gating only):

- `ttfx --music-ignore-preferences` → clap: `--music <SOURCE>` required.
- `ttfx --music /nonexistent.raw --music-ignore-preferences` → accepted, then
  `cannot read music fixture /nonexistent.raw`.
- `--help` lists the flag.

## Notes for the parent

- Stock (`run_music` not taken) path is untouched; the only global effect of the
  new arg is help/completion text.
- Parity, real-music comparisons and GUI checks are the parent's, as briefed.
