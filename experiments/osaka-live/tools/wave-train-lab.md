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

Nine irregularly spaced foam hands attach to the current outer lip, with
2 to 5 main fingers per hand. The shoulder hands are broad and the last hands
are smaller; deterministic root offsets, reach, taper and finger counts break
the old regular spine row. Blue water reaches the lip between hands. Each
hand and finger has a slightly offset blue underprint, painted before the
cream so the roots join cleanly. There is no continuous cream rim or separate
row of dark teeth. S0 and S1 have no foam or fingers; growth starts above 1.15
and completes at stage 4.

Thirty-four main fingers and four thin tip tendrils reuse the 38 spring slots.
Six bands run from shoulder lows to tip highs. Independent critical length
springs and underdamped flick springs (damping ratio 0.42) remain; onset
overshoot now changes forward reach. A shared positive lip flow transports
root order through the curl, with curvature and material-width limits.
Its arc-distance potential is computed once per crest; no path unions or
intersection searches run during rendering. Tips turn toward the water as
the flow relaxes beyond a hand. Reported angles use the forward root lip
tangent, signed clockwise in screen coordinates, rather than the outward
normal. Direction is also counted explicitly in screen coordinates.

The four tip tendrils have slender curved forks and intentional visual
overlap. Their lengths, widths and opacity open with highs, making more of
the lace visible when loud. They are excluded from main-finger crossing and
body-exclusion tests, but constrained separately to the final hand, finite
geometry within 220 px of their roots, main stroke width at most 4 px and
bounded opacity. Main fingers retain the 3 px crossing tolerance.
Main root edges may join their palm in the first quarter of their length;
body exclusion is enforced beyond that seam. The new 220 px geometry bound
allows forward hooks longer than the old 160 px outward spikes.

Whitecaps and contour flow are retained. Quiet coherent material wobble
continues with zero bands. The six profiles and body stage, bass height,
mid lean/throw and tempo travel mappings are unchanged.

Spray uses a fixed 192-slot deterministic xorshift pool. Band onsets and bass
hits can emit at most twelve droplets per event group, at most ten groups per
second. Each particle starts at a finger tip or crest root, inherits bounded
lip velocity, and receives a directional toss and wind. Gravity, mild drag
and shrink-out govern its lifetime (1.4 to 2.7 seconds). Pool exhaustion drops
new emissions. Every fourth fleck is drawn as a curved detached foam finger using the same
ballistic particle state. There is no collapse, collision, impact or crash state.

Sixteen contour ribbons advect through corresponding outer/inner profiles.
Their phase speed follows the existing energy/travel flow clock. Endpoint
opacity goes to zero before a row wraps, keeping the wrap invisible.

`response` holds the hero at S5 and 800 px amplitude while exercising quiet
(0.018, small 0.026 rises) and loud (0.42, repeated 0.65 onsets and kicks)
band inputs. It writes same-time quiet/loud frames and measured arc lengths,
end tangent angles relative to the forward root lip tangent, alive droplets,
maximum rooted clump depth, whitecap depth, contour displacement over two
seconds, and the silhouette's coordinate excursion over a twenty-second
frozen-stage study. These are isolated element controls, not a replacement
for the music fixture. Native motion frames show the quiet breathing.

The additional CTests cover exact lip and palm attachment, blue gaps, bounded
nonintersecting main-finger geometry and separately bounded overlapping tip lace, alternating long/short cells and opposite flicks,
per-band monotonic response and impulse isolation, deterministic pool
saturation/reuse/expiry, and dense stage and frame continuity. The existing
fixture tests remain in force. Passing tests does not establish artistic
acceptance or integrated world performance; world placement is W3.


## Lab clip camera

Only clip mode follows the selected travelling hero: its material anchor is
held at screen x=630 by a canvas translation. The pose, profile, envelope,
travel distance and music mapping are unchanged. The background stays in lab
coordinates. The CSV records world travel, hero anchor, camera x and projected
lip bounds every frame. Selection can move to the next periodic wave when
the current group recedes; this is a lab framing aid, not W3 world placement.
Bench mode retains the unmodified world coordinates and cost definition.
