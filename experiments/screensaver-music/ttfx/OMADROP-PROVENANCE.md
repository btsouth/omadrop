# Upstream reference

Source: https://github.com/omacom/ttfx

Tag: v0.5.0

Commit: 112ebb310b848d8f1251a5c2919a9cd5a5c78e18

This copy adds Omadrop's music control (see ../README.md): `src/music/`, the
music run loop in `src/fx/run.rs`, `--music*` options in `src/cli.rs` and
`src/main.rs`, a `cue` field on the engine, a cell-count variant of
`render_here`, and accent hooks in 27 effects' `next_frame`. The unmodified
upstream source is kept at `third-party/ttfx`; `diff -ru` against it shows
every change. Without `--music` the output is byte-identical to upstream.

LICENSE and NOTICE retain the ttfx and original TerminalTextEffects credits.
