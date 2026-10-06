# Kanagawa

A paper-print sunset: Fuji and a low red sun above a living indigo sea.
The sky, disc, snow cap, sea contours and broken foam come from the Journey
prototype. A standing wave frames Fuji from the left; the right foreground leaves room
for small boats.

The sky uses `gradient-sky-v1`, with a constant 0.72 paper wash. The existing Disc
uses independent sunset paints and soft bass rings; Mountain uses its original
snow cap at (1086, 476), base 616 and width 150. Their K1 settings are preserved.

`water-surface-v1` supplies twelve shaded depth planes and 35 localized sunset
reflection marks. Its `swellSeed` selects the same field as `swell-lines-v1` and
`foam-flecks-v1`. The three instances share seed 71, horizon 612, near depth 1112,
amplitude 0.85, drift 0.52 and perspective falloff 1.45. The old full-width inner
lines and water caps are disabled in this world.

600 broken swell contours vary in span and overlap, with thinner, denser marks
near the horizon. Their width range is 0.8..1.35 design pixels and length range
180..760 before depth scaling. The sea keeps moving in silence; individual
seeded drift rates and contour clocks avoid a short common loop. Bands 5 through
0 lift and brighten their own depth rows; kick supplies a small local surge.

240 candidate foam fragments use varied 0.3..1.1 size scales, lobed cream caps,
Prussian underprint and curled fingers. They follow the same moving swells.
Onsets and kicks brighten a sparse subset. Complete foam silhouettes are kept
out of x=-30..640, y=700..1120 and x=1080..1660, y=810..945, reserving wave and
boat space. Subtle sea contours continue through these areas as the water ground.
The region and exclusion rectangles are configurable in scene.json.

Palette: Kanagawa paper `#dcd7ba`, violet `#957fb8`, wave ink `#223249`, gold
`#e6c384` and red `#c34043`. The disc's paper-tinted reds reproduce the prototype's
faded sun; the sea retains its derived Prussian-blue paints. The finish uses the
prototype's paper fibre, grain 0.008, bloom 0.28 and vignette 0.25.

The stable composition removes Journey's chapter grades, sun travel and story
beats. Boat, crew and bird art are not part of this landscape yet.
Artwork and kit code use the repo's MIT license.

`great-wave-v1` sits at (0, 1100) on the left, width 1200, base height 650,
maximum added rise 210, curl 0.65, 18 forked crest claws and seed 71. It retains
the prototype body and flow family, with masked contours, Prussian underprint
and paper foam. The 1.6 second low-band envelopes ease its rise and settling;
kicks and onsets flick only the claws and spray. It never crashes. Even at
maximum rise, Fuji's summit and the right-hand boat reserve remain visible.
