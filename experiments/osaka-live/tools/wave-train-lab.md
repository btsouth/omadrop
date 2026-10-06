# Stage morph wave lab

`omadrop-wave-lab` is an offline surfaceless EGL study, separate from the
world loader and installed application. Build it through the osaka-live
`tests/CMakeLists.txt`. Its modes are `sweep`, `travel`, `response`, `bench` and
`clip`; all accept `--out`. The latter two require `--fixture`, stereo
44100 Hz float32 PCM with at least 60 seconds. `bench` requires UHD 770.

The hero has six authored profiles: soft swell, asymmetric building swell,
steep face, pitching lip, open hollow and full curl. Each has seven cubic
segments and 22 corresponding controls in normalized wave coordinates.
S5 retains the cubic water profile from Journey's `studyWave` in
`ending_water.cpp`; width and height are independently scaled. At default
full size it occupies approximately 55 percent of frame width and 75 percent
of frame height. W2 adds cream claws, free spray, broken foam and high-band whitecaps in this
lab only. The world registry and Osaka renderer do not use this piece.

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
height, lean, throw, lip spring, travel and group envelope. CPU timing includes the two causal body/detail controller hops per frame,
profile evaluation and canvas geometry/paint. It excludes the shared analyzer
and Score, which are outside the piece. The CSV also reports geometry/paint and
controller cost separately. GPU timing covers the isolated
piece upload, draws and layer composite at 1080p, excluding background,
finish, readback and encoding. Thirty frames are discarded as warmup.
`wave-train-*` CTests cover dense stage continuity, sample identity,
nonintersecting silhouettes, deterministic motion, bounded travel and real
fixture diversity. This lab does not establish owner acceptance or live
integrated world performance.

## Lip details and motion

Thirty-eight tapered claws are material strips attached to the current outer
lip. Their spacing is irregular and gets finer near the curl. Each has a
critical length spring and an underdamped flick spring (damping ratio 0.42).
The six analyzer bands run from the shoulder's lows to the curl tip's highs.
Band rises trigger their own flick, with a 180 ms refractory period; no other
band receives that impulse. A saturating band mapping and a smooth curvature
limit preserve room between adjacent fingers. Finger reach follows a common
streamline field in lip arc length, rather than independently bent straight
spikes. Occasional narrow forks stay in the same material cell. S0 and S1
have no claws; growth starts above 1.15 and completes at stage 4.

Foam hugs the inside of the lip. Its thickness uses local band energy, with
slow edge variation and curved carved breaks in the cream. Whitecaps on the
face and nearby swells use the high bands. All geometry has crisp edges and
uses the piece's existing palette. Quiet material wobble is coherent between
neighbouring controls and continues with zero bands. Loud energy increases
its displacement; it does not change the six authored profiles or the body
controller's stage, height, lean, throw and travel mapping.

Spray uses a fixed 192-slot deterministic xorshift pool. Band onsets and bass
hits can emit at most twelve droplets per event group, at most ten groups per
second. Each particle starts at a finger tip or crest root, inherits bounded
lip velocity, and receives a directional toss and wind. Gravity, mild drag
and shrink-out govern its lifetime (1.4 to 2.7 seconds). Pool exhaustion drops
new emissions. There is no collapse, collision, impact or crash state.

Sixteen contour ribbons advect through corresponding outer/inner profiles.
Their phase speed follows the existing energy/travel flow clock. Endpoint
opacity goes to zero before a row wraps, keeping the wrap invisible.

`response` holds the hero at S5 and 800 px amplitude while exercising quiet
(0.018, small 0.026 rises) and loud (0.42, repeated 0.65 onsets and kicks)
band inputs. It writes same-time quiet/loud frames and measured arc lengths,
end tangent angles relative to the lip outward normal, alive droplets,
maximum cream thickness, whitecap depth, contour displacement over two
seconds, and the silhouette's coordinate excursion over a twenty-second
frozen-stage study. These are isolated element controls, not a replacement
for the music fixture. Native motion frames show the quiet breathing.

The additional CTests cover exact lip attachment, bounded nonintersecting
parent/fork geometry, alternating long/short cells and opposite flicks,
per-band monotonic response and impulse isolation, deterministic pool
saturation/reuse/expiry, and dense stage and frame continuity. The existing
fixture tests remain in force. Passing tests does not establish artistic
acceptance or integrated world performance; world placement is W3.
