# Rotation and effect browser review — 2026-10-02

Installed requested update in the existing experiment. Normal command remains
~/.local/bin/omadrop -> ~/.local/share/omadrop-product/bin/omadrop.

## Result

All 37 actual Omarchy ttfx effects remain. Weighted shuffle bag offers every
visible effect once per round, avoiding the last min(8, enabled-1) selections
when candidates permit. Music intensity and stock duration weight order;
favorites get 2.5x priority, not extra frequency. Hidden effects leave rotation
and remain previewable. Settings reload at the next effect.

`omadrop --effects` / Omadrop Effects / third choice in `--choose` opens a
Zenity browser. Friendly names/descriptions, single and bulk favorite/hide,
selected previews, start rotation. Preview closes the current product session,
repeats one selected effect on one display, bypasses hidden settings, preserves
saved mode/display, waits for any-key exit then returns to the browser.
Curation file: ~/.config/omadrop/effects.conf, version 1. Locked delta merging
prevents stale browser snapshots erasing concurrent changes; atomic mode-600
writes preserve unknown keys/comments and refuse future formats.

V3 music-playing conductor parameters and all authored effects are retained.
A silent-clock bug was discovered and fixed: !playing reset fractional carry
every frame, preventing idle tempos below one tick/frame from progressing.
Now stale music debt clears at pause, fractional idle ticks accumulate.
This restores the documented idle drift and makes silent previews visible.

## Evidence

- Devbox cargo test --locked: 57 library tests, 11 integration tests, doc tests
  passed. Release build --locked passed.
- Six-minute recorded music check: 68 choices, all 37 in first round, no repeat
  within eight selections. Three-effect curation: 12 choices, exact enabled set,
  full round coverage, no adjacent repeats.
- All 37 effects x two seeds remain byte-identical to stock without --music.
- Recorded pause at 96.3..99.8 seconds: 419 frames, zero ticks. New unit check
  verifies a four-second hold then >200 idle ticks by 12 seconds.
- Real contained Ghostty silent preview: visible Beams; live capture path
  produced accumulating ticks and output rather than the previous black frame.
- Product mock integration and Python preference regressions passed: discovery,
  invalid arguments without side effects, all-hidden refusal, future version,
  cancel, actual pipe-separated Zenity selections, concurrent updates, class
  isolation, preview replacement and saved choices, compositor effect env,
  legacy commands, installs/backups and desktop entries.
- Contained browser lists 37; two checkbox favorites saved together. Hidden
  Beams preview launched; switching selected preview left only the new effect;
  saved mode=milkdrop and display=all remained. Any-key exit returned to browser.
- Desktop entries validate. User profile installed after checks, with no GUI
  launched on user's desktop and no curation defaults written.

Screenshots: ~/Documents/Codex/2026-10-02/omadrop-screensaver-review/rotation-browser/.
CSV/parity evidence: review/rotation-check/.

## Independent reviews

DS review 1002-085702-review found ambiguous preview flags, help accepted as
an effect, single-display fallback and --original toggle semantics; fixed.
Its multi-monitor class-mismatch claim is not supported by the code: all our
windows have the same class, so another product monitor is still recognized;
prior two-window contained check passed. Real multi-monitor layout remains an
existing unverified hardware limit. A second reviewer found stale preference
writes; locked delta merge and reproduction regressions fix that issue.

## Installed identity / recovery

New ttfx SHA-256:
ff80a95c5ee4f93258fcfda857390c62fa73012a1d1d45467fda708f4657cc83
MilkDrop renderer unchanged:
6e0530adf6850b1258fff20ad5727dcead0c9ef57a55b054e5205b6d1962e02c

Prior v3 runtime: ~/.local/share/omadrop-screensaver.v3-before-browser-20261002
Prior product: ~/.local/share/omadrop-product.before-browser-20261002
Historical baselines and prior normal launcher remain preserved. No commit,
publication or subjective artistic approval implied. Speaker/Bluetooth sync,
real hardware multi-monitor layout and full-speed user listening remain open.
