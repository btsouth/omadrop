# Native scene quality matrix

The user rejected this library's visual direction in live review. The
[music-connected prototype](music-connected-preview.md) rebuilds Ink Current
around evolving form and stronger musical response. The matrix below describes
the preceding checkpoint; its Ink Current row is not approval of the rebuild.

This is the working visual review for the current eighteen scenes. A scene is not
release-ready until its still, motion, transient roles, quiet behavior,
continuous material, ASCII material, palettes, and transitions all pass.

`Pass` below applies only to the evidence already reviewed. It is not a final
ship decision.

| Scene | Still | Response | Grammar | Color | Gray | ASCII | Main issue | Next review |
| --- | :---: | :---: | :---: | :---: | :---: | :---: | --- | --- |
| Bloom Engine | Pass | Pass | Selective | Pass | Pass | Pass | Keep petal growth distinct from directional thread motion | Two full songs pass; live output |
| Negative Space | Pass | Pass | Sparse | Pass | Pass | Pass | Preserve the dominant void when several roles overlap | Two full songs pass; live output |
| Centrifuge | Pass | Pass | Selective | Pass | Pass | Pass | Keep chamber accents subordinate to the fixed rotor and ejection rail | Two full songs pass; live output |
| Constellation Field | Pass | Pass | Sparse | Pass | Pass | Pass | Keep the fixed route and satellite branches legible when cues overlap | Two full songs pass; live output |
| Depth Tunnel | Pass | Pass | Flow | Pass | Pass | Pass | Keep deliberate depth travel broad while percussion remains local | Two full songs pass; live output |
| Glass Choir | Pass | Pass | Selective | Pass | Pass | Pass | Keep resonance, fractures, tip glints, and the conductor separate | Two full songs pass; live output |
| Ink Current | Pass | Pass | Selective | Pass | Pass | Pass | Keep event marks subordinate to the stable current | Two full songs pass; live output |
| Living Mosaic | Pass | Pass | Selective | Pass | Pass | Pass | Keep response inside selected cells and seams | Two full songs pass; live output |
| Lumen Fold | Pass | Pass | Selective | Pass | Pass | Pass | Preserve the calm installation while events use separate sheets, floor pools, cuts, and pins | Two full songs pass; live output |
| Orbital Loom | Pass | Pass | Selective | Pass | Pass | Pass | Keep the horizontal weave and local shuttles legible | Two full songs pass; live output |
| Paper Horizon | Pass | Pass | Sparse | Pass | Pass | Pass | Keep every response on a ridge, tear, star, lantern, or moon detail | Two full songs pass; live output |
| Particle Weave | Pass | Pass | Selective | Pass | Pass | Pass | Keep percussion on separate beads and knots, never the whole textile | Two full songs pass; live output |
| Prism Garden | Pass | Pass | Sparse | Pass | Pass | Pass | Keep roots, facets, crown dew, and the traveler visually separate | Two full songs pass; live output |
| Pulse Cathedral | Pass | Pass | Selective | Pass | Pass | Pass | Keep outer arches subordinate to the focal rose | Two full songs pass; live output |
| Shadow Architecture | Pass | Pass | Sparse | Pass | Pass | Pass | Keep percussion confined to separate architectural surfaces | Two full songs pass; live output |
| Spectral Ribbons | Pass | Pass | Flow | Pass | Pass | Pass | Keep sustained band contours and transient windows visually separate | Two full songs pass; live output |
| Tidal Grid | Pass | Pass | Selective | Pass | Pass | Pass | Keep foreground grid lines below the horizon subject | Two full songs pass; live output |
| Wire Organism | Pass | Pass | Sparse | Pass | Pass | Pass | Keep root, branch, and crown cues separate inside the fixed body | Two full songs pass; live output |

## Current priorities

1. Generate a new anonymous full-library review after the Orbital Loom,
   Constellation Field, Wire Organism, Prism Garden, Glass Choir, and Centrifuge
   rebuilds. Record decisions before revealing scene names.
2. Review continuous motion at native display size and confirm that Depth
   Tunnel and Spectral Ribbons remain controlled as the two deliberately broad
   flow scenes. Any still, motion, or music score below 4 out of 5 requires
   revision.
3. Run the complete profile and real-song gates after every review-driven scene
   change, then run one exact `bin/omadrop-check --full` release gate with a
   fresh 10-minute soak.
4. Use direct deaf and hard-of-hearing study results to judge whether the
   musical mappings are understandable. Do not present renderer measurements
   as accessibility proof.
5. Expand the rights-cleared real-song set only when a source adds meaningful
   acoustic, vocal, or heavily compressed coverage. The current two songs and
   five generated profiles already pass all 18 scene gates.
6. Do not add scenes for this release. Reconsider expansion only when a new
   composition and motion grammar are clearly different from this matrix.

## Motion grammar

- Sparse scenes keep non-transient frame coverage near or below 12 percent and
  reserve most changes for small objects or lines.
- Selective scenes keep non-transient coverage below 18 to 30 percent and
  assign percussion roles to different regions or structures.
- Flow scenes may sustain motion across as much as 45 to 50 percent of the
  frame. No more than one third of the library may use this grammar.

A scene fails the automated quality floor when it exceeds its coverage budget.
Beat, kick, snare, and hat motion must each reach at least 1.75 times quiet
motion. An isolated kick, snare, or hat must also change at least 0.025 percent
mean frame luminance. This absolute floor prevents a nearly motionless baseline
from turning an invisible gesture into an impressive response ratio.
It also fails when sustained low, middle, or high-frequency material disappears
between attacks, or when those groups change the same pixels. This keeps the
song readable without requiring the scene to bounce on every transient.
Full-frame brightness and scale pulses are not valid substitutes for distinct
kick, snare, and hat gestures. Constant bounce, jitter, and whole-composition
thumping also fail review even when they remain below an automated threshold.
Broad motion is reserved for the few scenes whose visual identity requires it;
ordinary audio must not move the whole composition. The global-pulse score
multiplies changed pixel coverage by same-direction luminance coherence and has
a scene-specific limit.
Pulse duty measures how often that score exceeds 10 and 20 percent. Sparse,
selective, and flow scenes may spend at most 25, 45, and 48 percent of frames
above the moderate 10-percent level. Sparse and selective scenes may spend at
most 12 percent above the severe 20-percent level; flow scenes may spend at
most 25 percent there. A scene also fails when kick, snare,
and hat all change a similarly broad part of the image, or when their gestures
recover so slowly that repeated hits become constant motion. On the current
full-song review, every sparse and selective scene stays at or below 4 percent.
The complete 90-combination default suite and two full rights-cleared songs
pass both pulse-duty limits. Separate 18-scene structured-track runs pass at
both 125 percent
response intensity and with reduced motion enabled, including the same pulse,
recovery, role-separation, and silence gates.
Spectral Ribbons now reports zero frames above the 20 percent broad-pulse level
on all five locked profiles after assigning percussion to separate local
windows and removing raw spectrum changes from its full-width geometry.

The anonymous motion review exposed Bloom Engine and Orbital Loom as the
closest motion pair. Orbital Loom now uses six offset horizontal threads,
directional feedback transport, local beat and anticipation shuttles, separate
kick knots and snare heddles, and no shared snare rotation or expanding onset
rings. All five replay profiles and both complete approved songs pass with the
new motion. Across the real tracks, moderate whole-frame pulse duty stays below
0.5 percent and severe pulse duty stays below 0.02 percent.

The still review exposed Constellation Field's previous striped links as three
oversized rods rather than a coherent network. Its replacement is a fixed,
hand-composed route with thin curved links and four satellite branches. Kicks
light three low nodes, snares dash only the central bridge, hats activate the
right satellites, and one marker carries beat position through the route. All
five replay profiles and both complete approved songs pass with zero moderate
or severe broad-pulse frames.

The same still review exposed Wire Organism's previous single stroke and large
attached rings as too thin beside the newer scenes. Its replacement is a fixed
translucent body with a persistent spine, inner current, eight side branches,
three root filaments, and five crown cilia. A beat marker travels along the
spine while kicks use the roots, snares use middle branches, and hats use the
crown. The composition stays fixed instead of bending, scaling, or pulsing as
a whole. All five replay profiles and both approved full songs pass with zero
moderate or severe broad-pulse frames.

Prism Garden's previous repeated bars and single-diamond tops read as an
equalizer rather than an authored place. Its replacement is a fixed garden of
seven differently proportioned crystal plants with paired leaf blades,
faceted crown clusters, connected surface roots, and separate underground
filaments. Kicks illuminate three root bulbs, snares articulate selected
facets, hats place dew on crown tips, and one pollinator carries beat position
across the skyline. Low, middle, and high sustained material remains visible
in roots, leaves, and crowns without moving the plants. All five replay
profiles and both approved full songs pass with zero moderate or severe
broad-pulse frames.

Glass Choir's previous five thick neon diamonds read as flat clip art rather
than suspended glass. Its replacement arranges seven slender asymmetric
voices at different heights, widths, and angles, with thin bevels, internal
facets and caustics, suspension threads, and subdued reflections. Kicks ring
three lower chambers, snares fracture two selected voices, hats glint their
tips, and one conductor carries beat position through the ensemble. Sustained
frequency groups illuminate separate glass details without changing the fixed
silhouettes. All five replay profiles and both approved full songs pass with
zero moderate or severe broad-pulse frames.

Centrifuge's previous full-screen concentric rings, spokes, square frame, and
feedback rotation duplicated the radial language of stronger scenes and made
continuous spinning the subject. Its replacement is an asymmetric physical
rotor with six fixed arms and sample chambers, a broken rim, a stable hub, and
a curved ejection rail. Kicks ring two weights, snares extend opposing clamps,
hats use short rim ticks and selected samples, and one counterweight carries
beat position around the machine. All five replay profiles and both approved
full songs pass with zero moderate or severe broad-pulse frames.

Run the anonymous final review without exposing scene names:

```bash
OMADROP_MOTION_REVIEW_WIDTH=1920 \
OMADROP_MOTION_REVIEW_HEIGHT=1080 \
  bin/native-motion-review --anonymous song.f32 /tmp/omadrop-scene-review 26 18
```

The command preserves analysis from the beginning of the song, encodes only
the requested window, and places the scene mapping under `truth/` for the
facilitator to reveal after still and motion decisions are recorded.
