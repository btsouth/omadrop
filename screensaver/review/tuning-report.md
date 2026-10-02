# Initial worker conductor report (historical)

Superseded by the final review and installed defaults described in
`final-review.md`. The launcher uses 120 stock ticks/s, not 60; the parent
corrected the rates, banked-motion silence handling and first resumed hit.

Scope: `ttfx/src/music/mod.rs` and `ttfx/src/fx/run.rs`. The stock path
(`run_effect`, no `--music`) and all 37 effects / authored art are untouched;
with music off the binary is stock ttfx. Only the conductor's mapping from
measurements to the effect clock changed.

## What changed

### Correctness

1. **Frame-rate-independent drift/delivery.** Continuous terms are now rates in
   ticks per second, multiplied by the frame's `dt`; the move ease was already
   `1 - exp(-dt/ease)`. The old code added `drift` (and `idle`) straight onto
   the per-frame carry, so the same music ran ~2x faster on a 120 Hz loop than
   on a 60 Hz one. `Tuning`'s docs now state units (rates ticks/s, sizes ticks,
   times seconds; one tick = one stock frame, so 60/s is stock speed).
2. **The per-frame cap no longer drops ticks.** The cap (`MAX_TICKS_PER_FRAME`
   = 12) moved into `Music::advance`, which banks any surplus in the carry for
   later frames. `run.rs` no longer truncates the return value with `.min(12)`
   after the debt was already subtracted.
3. **Stale accents through silence.** Not playing now clears `accent` (and
   `owed`), so a hit heard just before a pause cannot fire on resume.
4. **The finishing clock counts music played, not wall time.** A new
   `effect_paused` accumulator is subtracted from the effect age, so a pause
   cannot age an effect toward its finish boost.
5. **Stale ranking history on resumed music.** On a pause -> play transition,
   `hits`, `hit_flux` and `last_move` reset, so a new song is not ranked against
   the previous one.
6. **Invalid tuning values.** `Tuning::sanitized` clamps every field to a
   runnable range (and replaces NaN/inf), and `from_env` applies it. A
   `move_ease=0` from `OMADROP_MUSIC_TUNING` can no longer divide by zero,
   `move_rank` cannot invert, and `max_tempo` cannot run away.
7. **Overdriven flashes.** `Cue::burst` growth is clamped to the documented
   2-4x band, so a very strong accent does not spawn ~6x launches.

### Tuning (defaults)

| field | old (per 120 Hz frame) | new | rationale |
|---|---|---|---|
| `drift_floor` | 0.2 | 12 ticks/s (0.2x stock) | meaningful minimum progress while quiet music plays, so a slow song still completes |
| `drift_gain` | 0.25 | 15 ticks/s | modest: fast songs are carried by spaced moves, not high average speed |
| `move_base` / `move_gain` | 30 / 22 | 30 / 22 ticks | bounded move (~0.5 s of authored motion), size scaled 0.7-1.6 |
| `move_ease` | 0.05 s | 0.12 s | smooths a move over ~0.3 s and keeps the first frame's share under the cap |
| `move_spacing` | 0.2 s | 0.28 s | at most ~3.5 standout moves/s, so fast music reads as distinct moves |
| `finish_after` / `finish_rate` | 10 s / 0.1 | 10 s / 6 ticks/s per s | long effects accelerate to completion |
| `max_tempo` | 12 per frame | 90 ticks/s (1.5x stock) | ceiling on the drift rate, including the finish boost |
| `idle_after` / `idle_tempo` | 6 s / 0.35 | 6 s / 21 ticks/s (0.35x stock) | unchanged intent (slow drift after a stopped player), now frame-rate correct |

`OMADROP_MUSIC_TUNING` keys are unchanged, but the drift/idle/finish numbers
are now per second rather than per 120 Hz frame.

## Tests added (`src/music/mod.rs`, `#[cfg(test)]`)

State-level tests over a zero-length fixture; no audio is synthesized.

- `drift_and_moves_pace_the_same_at_any_frame_rate` - same 2 s of state at 60
  and 120 Hz delivers the same ticks (within rounding).
- `the_frame_cap_banks_move_ticks_instead_of_dropping_them` - a 500-tick debt
  survives the 12/frame cap and is fully delivered.
- `silence_freezes_and_drops_a_stale_accent` - silence yields 0 ticks and
  clears the pending accent and move debt.
- `the_finish_clock_counts_music_played_not_wall_time` - 30 s paused does not
  age the effect's finish clock.
- `quiet_playing_music_still_progresses` - quiet playing music delivers at
  least the drift floor in 1 s.
- `resumed_music_forgets_the_old_ranking` - a pause/resume clears the hit
  ranking history.
- `invalid_tuning_values_are_clamped` - NaN/negative/out-of-range tuning is
  clamped.

## Commands and results (all on devbox-bts)

```
rsync -a --exclude target --exclude .git \
  experiments/screensaver-music/ttfx/ \
  devbox-bts:/workspace/worktrees/omadrop-omarchy-flagship/experiments/screensaver-music/ttfx/

ssh devbox-bts 'cd .../ttfx && PATH="$HOME/.cargo/bin:$PATH" cargo test --locked'
# 42 lib unit tests (7 music) + integration binaries: all ok, 0 failed

ssh devbox-bts 'cd .../ttfx && PATH="$HOME/.cargo/bin:$PATH" cargo build --release --locked'
# Finished `release` profile [optimized] in 34.92s

rsync -a devbox-bts:.../ttfx/target/release/ttfx \
  experiments/screensaver-music/build/ttfx-music
# ELF 64-bit x86-64, 3320736 bytes

printf 'abc' | build/ttfx-music --frame-rate 0 print   # -> exit 0 (stock path)
build/ttfx-music --version                              # -> ttfx 0.5.0
```

## Limits of validation

- These are **correctness/behavioral** checks, not perceptual or aesthetic
  acceptance. No fixtures were available locally, so no real-music run was made
  and no visual quality is claimed.
- The tests exercise the conductor's state machine directly; they do not prove
  how the pacing reads on screen, which still needs an audio review with a real
  fixture and the parent's desktop-specific checks.
- The tuning numbers are reasoned from the stated goals (bounded moves, wide
  spacing, minimum progress, capped rate); they are starting points for that
  review, not a validated best.
