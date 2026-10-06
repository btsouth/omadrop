# Stage morph wave lab

`omadrop-wave-lab` is an offline surfaceless EGL study, separate from the
world loader and installed application. Build it through the osaka-live
`tests/CMakeLists.txt`. Its four modes are `sweep`, `travel`, `bench` and
`clip`; all accept `--out`. The latter two require `--fixture`, stereo
44100 Hz float32 PCM with at least 60 seconds. `bench` requires UHD 770.

The hero has six authored profiles: soft swell, asymmetric building swell,
steep face, pitching lip, open hollow and full curl. Each has seven cubic
segments and 22 corresponding controls in normalized wave coordinates.
S5 retains the cubic water profile from Journey's `studyWave` in
`ending_water.cpp`; width and height are independently scaled. At default
full size it occupies approximately 55 percent of frame width and 75 percent
of frame height. Claws and spray belong to the next stage of work.

The profiles are sampled once, then Catmull-Rom interpolated at 112 fixed
boundary samples. Bezier linearity permits a small per-control displacement
for slow deterministic wobble and lip lag without resampling the base paths.
Contours interpolate between corresponding outer and inner spline samples,
so they follow the face and wrap into the hollow. Filled ribbons reuse fixed
quad connectivity. There are no path unions or point-containment queries.
The surrounding swells retain a four-component Gerstner sum.

Band energy and swell drive a critically damped stage spring. Bass drives a
separate height spring; mids drive lean and lip throw. An underdamped spring
moves the lip controls with lag and modest overshoot. Recent onset intervals
estimate tempo and control travel speed. Motion advances at analyzer hops,
independent of frame rate. Travelling instances recede under the periodic
Gaussian group, which reduces stage as well as height before departure.
No impact, plunge, ballistic fall or crash state exists.

The CSV records both the music-driven stage and the drawn hero stage, plus
height, lean, throw, lip spring, travel and group envelope. CPU timing covers
profile evaluation and canvas geometry/paint. GPU timing covers the isolated
piece upload, draws and layer composite at 1080p, excluding background,
finish, readback and encoding. Thirty frames are discarded as warmup.
`wave-train-*` CTests cover dense stage continuity, sample identity,
nonintersecting silhouettes, deterministic motion, bounded travel and real
fixture diversity. This lab does not establish owner acceptance or live
integrated world performance.
