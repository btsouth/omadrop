# Wave train W1 lab

`wave-train-v2` is a generic experimental C++ piece and a surfaceless EGL lab.
It is not registered in the world loader. Production Kanagawa still uses
`great-wave-v1`; Osaka, the production executable and both worlds are unchanged.
No desktop, live audio capture or PipeWire service is opened by this tool.

## Build and capture

From the repository root, reuse the renderer test build:

```sh
cmake -S experiments/osaka-live/tests -B experiments/osaka-live/build-tests \
  -DCMAKE_BUILD_TYPE=Release -DWAVE_TRAIN_FIXTURE=/path/three-tracks-180s.f32
cmake --build experiments/osaka-live/build-tests \
  --target omadrop-wave-lab wave-train-tests -j6
ctest --test-dir experiments/osaka-live/build-tests -R wave-train --output-on-failure
```

Run `experiments/osaka-live/build-tests/omadrop-wave-lab` with `--out DIR`:

- `--mode sweep`: 12 full-resolution frames, fixed 330-pixel amplitude,
  Q from 0.2 through 1.6; phase, group position and lip throw are fixed.
- `--mode travel`: 30 frames at seconds 0 through 29, fixed controls and
  88-pixel/second phase speed, 44-pixel/second group speed.
- `--mode clip --fixture FILE`: the first 60 seconds through the production
  streaming analyzer at 735-frame hops, rendered at 1080p30. H.264/yuv420p
  video includes AAC audio from the same interval of the supplied fixture.
- `--mode bench --fixture FILE`: the same 60-second controls, piece alone at
  1080p on devbox UHD 770, without background, finish, readback or encoding.

Each mode writes a parameter CSV. Clip and bench record 1,800 frames. Bench
excludes the first 30 frames from the summary. OpenGL elapsed queries cover
vertex upload, drawing, the layer and its composite; CPU geometry generation
is reported separately. This is GPU execution cost, not display presentation.
`--background FOLDER` reads the current Kanagawa sky and sea settings from
`scene.json`; no other world slot is rendered. `make-wave-strips.py DIR`
composes the sweep and travel frames into annotated contact strips.

## Surface and loop resolution

The dominant component uses normalized Gerstner coordinates:
`x = a - Q/k * sin(theta)`, `y = waterline - amplitude * cos(theta)`.
Q is nondimensional horizontal steepness, with the fold at Q=1. This is the
specified trochoid with A=1/k in horizontal sea coordinates; the independent
vertical print scale follows bass without changing wavelength. Three smaller
components use noninteger wave numbers and deep-water dispersion, omega
proportional to sqrt(k). There is no reference silhouette or scaling of v1.

Periodic Gaussian groups travel at half the integrated phase speed. Local
amplitude and Q both follow the group, so a particular travelling crest grows,
curls and recedes. A smooth right-side falloff favors the left-side hero;
the lab reports the strongest crest within x=0..960 as hero without using a
hard selection switch to animate the surface.

The supported sheet caps horizontal Q at 0.94. Above local Q=1, solve the
actual loop crossing `u = Q*sin(u)` by bisection. Union its upper lobe with
the sheet, then expose the forward branch as a finite-thickness pitched lip.
The lip ends at 1.28 times the fold angle, before the returning intersection.
Throw saturates at the fold with zero slope, preserving the inward turn even
on strong mid energy. A final exterior reconstruction fills closed stitching slivers: the hollow
is open toward the trough, never a closed hole or detached water island.
CTests check one connected, finite, simple exterior across the complete sweep,
travel and real fixture, and confirm the raw diagnostic curve really loops.

The lip stays at least 30% of its local amplitude above the waterline over the
supported range. The group carries the crest down; there is no ballistic
plunge, impact, crash phase, foam finger or spray state in W1.

## Music and print motion

All measurements are causal and use the existing analyzer. Each control has
its own exact critically damped spring, advanced at audio hops:

| Control | Target | Spring omega |
| --- | --- | --- |
| Amplitude | 100 + 340 * bassLevel / (0.20 + bassLevel) pixels | 1.05 * beat Hz |
| Q | 0.2 + 1.4 * energy | 1.55 * beat Hz |
| Phase speed | 42 + 22 * beat Hz + 12 * onset density pixels/s | 0.65 * beat Hz |
| Lip throw | 165 * mean(mid bands) / (0.18 + mean(mid bands)) pixels | 2.25 * beat Hz |

Energy is clamped `3.1 * RMS(six bands) + 0.35 * surge`. Tempo is the median
recent inter-onset interval folded into 60..180 BPM, defaulting to 90 BPM.
Onset density is the bounded count in the previous eight seconds divided by
16. Phase distance is integrated rather than recomputed from current speed.

Depth contours derive from the moving surface with orbit attenuation and
secondary depth lag. Their depth phases advect upward at an energy-dependent
speed and fade before wrapping. Crown ink derives from the same physical lip.
The depth gradient, blue underprint, foam and line colors match Kanagawa.
The crest foam band uses slope and height, widens with Q, and blends into the
lip band continuously. W2 can add the individual foam fingers and spray.

These checks establish lab geometry and response. They do not establish
owner aesthetic acceptance, production integration or live playback acceptance.
