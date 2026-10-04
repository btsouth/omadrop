# Osaka Jade live renderer

Live Osaka renderer extracted from Journey commit `4c2d413` onto
`product`. It includes Osaka, the vector canvas, compositor, rigs and sign
outlines. It contains no crossing, Kanagawa, road ending or chapter titles.
The dispatcher installs this as bin/omadrop-osaka and uses it for Omarchy mode
on every theme. MilkDrop remains the default.

Build on devbox with `qmake6 osaka.pro` in a build directory, then `make -j8`.
Run GUI previews only in omabox. Escape closes all Osaka windows and returns to the app controls.
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
can launch the existing fireworks after a seeded 45 to 90 second cooldown. A three-second smoothed RMS
before the quiet-music gain chooses the show size: below 0.09 gets one shell
at 35% radius, without the secondary shell or follow-ups. At 0.09 and above,
the full show retains follow-ups requiring newly measured events. This
threshold separates fixture means of 0.066 (Gymnopedie), 0.112 (Sneaky Snitch)
and 0.375 (Volatile Reaction). There is no silent fireworks fallback.
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

--single uses the first display, as MilkDrop does; --all uses every Qt screen.
The explicit choice shares MilkDrop display preferences and preserves newer
preference versions. Windows share one live audio session and loop indefinitely.
Real audio devices and physical multi-monitor behavior require hardware testing.
