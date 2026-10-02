# Installed tuning and modes, 2026-10-02

User authorized implementation after proposing one product with Omarchy,
MilkDrop, and eventual Originals modes, and separate ASCII presentation.
Feedback: v2 is "pretty good"; slow songs take too long to finish effects;
fast music still hides meaningful changes. This revision needs their viewing.

## Implemented

- Shared `omadrop` command, remembered mode in `~/.config/omadrop/mode.conf`,
  `--choose` and an Omadrop Modes desktop entry. Initial/default mode is MilkDrop.
- `--mode milkdrop|omarchy`, `--ascii` / `--no-ascii` for MilkDrop,
  shared single/all display preference, cross-mode stop/switch and normal toggle.
  Chooser cancellation or failed validation leaves state/current session intact.
- MilkDrop backend, all presets, and normal config remain preserved. ASCII uses
  its existing renderer option. Omarchy uses actual ttfx authored characters.
  Separate backend windows are intentional; no in-window mode switch is added.
- No Originals placeholder or rejected native scenes. Future custom content must
  earn acceptance before that mode exists.
- Omadrop terminal class is `org.omadrop.screensaver`; per-launch fullscreen/float
  rules replace reliance on the stock class. Cleanup stops its own music child
  and product windows. Stock Omarchy and unrelated ttfx are excluded.
- Conductor moves: 30 + 22*intensity ticks, strength scaled 0.7..1.6, 120 ms
  exponential easing, at least 280 ms between standout hits. Ordinary hits nudge.
  Continuous floor 60 ticks/s (0.5x at 120 Hz); gain 12 ticks/s; finishing starts
  after 8 seconds of playing music, adds 9 ticks/s per extra second, capped drift
  180 ticks/s. Silence clears motion/accent/carry; pauses do not age finishing.
- Drift uses elapsed time; the per-frame cap banks earned ticks. Ranking resets
  before the first resumed hit. Invalid tuning values are sanitized.

## Evidence

Rust build and complete ttfx tests ran on devbox, all passed (42 library tests,
including seven conductor regressions, plus integration/golden tests). Final
log: Documents/Codex/2026-10-02/omadrop-screensaver-review/v3/rust-tests-final.log.
Stock parity: 37 effects x two seeds, byte-identical without music.

Same 124-second actual-music fixture, seed 16, 137x33 terminal, 120 Hz:

| Section | installed v2 stock speed | v3 stock speed |
|---|---:|---:|
| Kid Quill, 0..36 | 1.85x | 1.73x |
| fast section, 36..66 | 2.64x | 1.98x |
| piano, 66..96 | 0.35x | 1.28x |

V3 delivers ~2 and 2.27 standout accents/second in the fast sections, 1.13 in
piano. Zero ticks in the 96.15..99.9 inserted pause window. Pace and scene
completion are engineering measurements, not aesthetic scores. Longer effects
still have their authored duration and are not forcibly cut off.

Contained desktop checks: actual chooser launches fullscreen Omarchy; normal
toggle and Escape close it; MilkDrop continuous and ASCII render; switching
closes the previous backend; stock-class terminal survives our --stop. Two
virtual outputs retained two fullscreen product windows, then --stop removed
both. The extra headless output had no usable mode, so two-display visual layout
was not established. GUI used omabox only; MilkDrop needed software GL/Xwayland
inside the box. Real GPU/speaker/Bluetooth sync is not newly verified.
Mock integration checks cover default/remembered mode, both chooser cancellations,
ASCII env, invalid args, missing backend without side effects, legacy forwarding,
stop/switch isolation and install/backup/reinstall behavior. All passed.

Preview: `Documents/Codex/2026-10-02/omadrop-screensaver-review/v3/preview-v3.mp4`,
50 seconds, actual Ghostty frames replayed at 60 fps with matching audio. It uses
fixture seconds 54..104: fast music, piano, four-second pause and return. Full
terminal runs and logs remain alongside it. The capture helper now copies inputs
into the contained HOME rather than assuming host Documents is mounted.

## Installation and recovery

Installed ttfx-music SHA-256:
`b84f4bbbcb5f94895d2b6b422d11f777eb7cba21b1c0276d928a03df4c8639e1`.
Preserved MilkDrop renderer SHA-256:
`6e0530adf6850b1258fff20ad5727dcead0c9ef57a55b054e5205b6d1962e02c`.

Normal command points at `~/.local/share/omadrop-product/bin/omadrop`;
old link is `~/.local/bin/omadrop.before-product` and still launches accepted
MilkDrop directly. Prior screensaver runtime:
`~/.local/share/omadrop-screensaver.before-modes-20261002`.
Screensaver binary replacement uses a temporary file plus atomic rename.

Known existing silence behavior is retained: after six seconds of silence the
screensaver resumes idle drift (42 ticks/s). Audio alone does not distinguish a
long pause from stopped playback. The four-second pause stays still. Mode chooser
is a basic zenity dialog, not a new renderer. No publication, commit or aesthetic
acceptance is implied. Source and evidence are mirrored to the devbox worktree.

Final media validation: 50.000 seconds, 3000 video frames, audio 50.000 seconds.
Decoded frames at clip seconds 42.3..45.6: 198 frames, one unique image hash.
Fast (2..5 s) and piano (17..20 s) frame strips were inspected and saved alongside
the video. Final installed build was installed into the box and Escape rechecked;
its window closed and no ttfx-music process remained. No subjective full-speed
listening verdict is claimed.
