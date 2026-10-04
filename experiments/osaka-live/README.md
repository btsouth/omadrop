# Osaka Jade live renderer

Standalone integration candidate extracted from Journey commit `4c2d413` onto
`product`. It includes Osaka, the vector canvas, compositor, rigs and sign
outlines. It contains no crossing, Kanagawa, road ending or chapter titles.
The product controller, dispatcher, MilkDrop, ttfx and packaging are unchanged.

Build on devbox with `qmake6 osaka.pro` in a build directory, then `make -j8`.
Run GUI previews only in omabox. Escape closes this standalone preview.
Launching it without a duration runs indefinitely.

Audio uses the existing `PipeWireCapture` at 44100 Hz stereo. The analyser
processes 735-frame hops, preserving the six frequency bands, independent
stereo analysis and original attack/release responses. A limited shared gain
helps quiet music while preserving frequency balance; silence closes the
activity gate. There is no file score or future-event search. Events are
stamped when discovered, with at most one hop of peak confirmation. Recent
event history and strand phases are bounded.

The camera stays in Osaka. Train, cyclist, gust, tea, cooking, toast and small
sky actions have separate seeded schedules. Smaller gestures and window wakes
have independent periods. Strong newly measured onsets, bass attacks or surges
can launch the existing fireworks after an 18 to 30 second cooldown. Follow-up
shells also require new measured events. There is no silent fireworks fallback.
Birds fly away, leave the viewport and return separately. Ambient movement
continues through silence. A monotonic double clock never rewinds the scene;
the strand phases wrap only at their common continuous repeat period.

`--probe` measures the streaming response without drawing. `--bench` measures
drawing without encoding. `--record out.mp4` records the same live session in
real time using surfaceless EGL. Recording defaults to 60 seconds at 1080p30;
`--fps 60` is also supported. `--stats out.csv` records response and timing.
The existing `OMADROP_PW_RECORD_COMMAND` test seam can feed paced fixture PCM
where system audio is unavailable. This does not verify a real audio device.

Configure `tests/` with CMake and run its CTest checks on devbox. They cover
causal prefixes, chunk boundaries, opposite-phase stereo, silence release,
eight simulated hours of bounded state, event cooldowns and the original
canvas geometry digest. `--verify-render` checks exact RGB compatibility of
the canvas optimization on three controlled poses, including fireworks.

This is milestone 1. App integration, ttfx removal, a fresh package install
and a capture of the installed app require Brandon's review of the live loop.
