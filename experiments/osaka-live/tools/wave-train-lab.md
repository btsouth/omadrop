# Stage morph wave lab

`omadrop-wave-lab` is an offline surfaceless EGL study, separate from the
installed application. Its default study parameters remain independent of
the Kanagawa world integration. Build it through the osaka-live
`tests/CMakeLists.txt`. Its modes are `sweep`, `travel`, `response`, `bench` and
`clip`; all accept `--out`. The latter two require `--fixture`, stereo
44100 Hz float32 PCM with at least 60 seconds. `bench` requires UHD 770.

The hero has six authored profiles: soft swell, asymmetric building swell,
steep face, pitching lip, open hollow and full curl. Each has seven cubic
segments and 22 corresponding controls in normalized wave coordinates.
S5 retains the cubic water profile from Journey's `studyWave` in
`ending_water.cpp`; width and height are independently scaled. At default
full size it occupies approximately 55 percent of frame width and 75 percent
of frame height. W2 adds cream claws, free spray, broken foam and high-band whitecaps.
Kanagawa registers the same piece as `WaveTrain` / `wave-train-v2`; Osaka
Jade retains its existing pieces.

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

The claws follow the Journey K3 prototype frame (`kanagawa.cpp` and
`direction_study.cpp`): cream sickle claws rooted on the current outer lip,
heading forward and down over the face and hooking to a point. Each claw has a
short pointed blue claw rising behind it in the underprint colour, so the
crest silhouette is ragged blue against the sky with cream hooks below. A
thin ragged cream sheet joins the roots, deeper under the clusters, with blue
lace cavities inside it.

The 38 spring slots are 38 main claws in eleven clusters of three or four.
Spacing, reach, width, hook and drop vary by deterministic hashes, and about a
third of the claws are slender, so no regular row forms. Some carry a thin
side hook from the middle. Width is limited to about a quarter of the reach,
so short claws never fold back over their root. The crest top foams first:
shoulder and tip clusters open later with stage. S0 and S1 have no foam or
claws; growth starts above 1.15 and completes near stage 4.

Six bands run from shoulder lows to tip highs. Each claw keeps its critical
length spring and underdamped flick spring (damping ratio 0.42). Louder bands
reach further, wider and slightly more forward; an onset flicks a claw out and
up with overshoot, then it settles back. A slow per-claw sway keeps quiet
water moving. Reported angles use the forward root lip tangent, signed
clockwise in screen coordinates; at rest every claw hooks forward and down
(angle between 0 and pi).

The curl tip unravels into seven thin tendrils that curl both ways and overlap
on purpose; they lengthen with the highs. Six loose claws drop from the curl
into the barrel on the flow clock and fade in and out, so the cycle never pops.
Claws overlap each other like the woodblock reference and may lie over the
face, so there is no body-exclusion or crossing requirement; each claw outline
must still be simple and bounded.

## Woodblock finish (P1)

The body is printed as five fixed tone bands between the outer skin and the
face, dark outside and lighter through the middle, each with a top-to-base
gradation. Thin light key lines separate the bands. Of the sixteen flowing
contours, four are tapered cream veins on the upper face and into the curl and
eight are faint lines; the rest are not drawn. A dark key line outlines the
silhouette, the foam sheet's rim, every claw, side hook and tip tendril, in
place of the earlier offset blue peaks, which only read against a cream sky.
Clusters sit between 22% and 87% of the lip, so the back of the wave stays
smooth; the sheet fades out there too. Face whitecap chips are no longer
painted. Five thicker tip tendrils all curl into the barrel.

The near water is painted after the crests, so the wave rises out of the sea.
A smaller foreground swell rises in front of the hero's foot, sized to cover
it, with a broken cream crest and small sickle claws spilling down its front.

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
maximum foam sheet depth, whitecap depth, contour displacement over two
seconds, and the silhouette's coordinate excursion over a twenty-second
frozen-stage study. These are isolated element controls, not a replacement
for the music fixture. Native motion frames show the quiet breathing.

The additional CTests cover exact lip and sheet attachment, cluster counts,
bounded simple claw and side-hook outlines, blue claws anchored to their roots,
forward-and-down hooks at rest, bounded tip tendrils and loose claws,
alternating long/short cells and opposite flicks,
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

## Kanagawa world integration

The `great-wave` instance uses `WaveTrain` with explicit scene parameters. A
value-owned controller advances at production analyzer hops in `Schedule`;
rendering and boat support read that snapshot. World changes atomically
publish the train parameters. Worlds without a train do not advance its
controller. Spray emission samples the same configured profile it paints.

Kanagawa scales height to 0.88 and spatial travel to 0.2. The authored profiles,
claw controls, spring rates, band response and tempo estimation retain the lab
mapping. The travelling group uses a 1800 px period, 1500 px width and 0.94
minimum group envelope before the existing departure taper. These placement
parameters keep a substantial set in view while the original material flow
and sea waves continue. The world surge raises the stage spring target to S5;
release settles through the same spring without a collapse state.

Boats resolve their named train and retain their original depth ordering.
Support samples the drawn sea polyline and the upper water boundary, with the
existing depth blend for the far boat. Hull angle, keel fit, wake and oar
contacts share that field. Escape follows the slower body lip with space
reserved for the hooked details and face/sea junction; independent claw
flicks do not jerk the hull. Clearance checks the actual detail bounds. The
standalone world foam-flecks layer is removed because the hero owns its foam,
whitecaps and spray. The original great-wave piece and its tests remain.
