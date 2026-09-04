# Native scene quality matrix

This is the working visual review for the current eighteen scenes. A scene is not
release-ready until its still, motion, transient roles, quiet behavior,
continuous material, ASCII material, palettes, and transitions all pass.

`Pass` below applies only to the evidence already reviewed. It is not a final
ship decision.

| Scene | Still | Response | Grammar | Color | Gray | ASCII | Main issue | Next review |
| --- | :---: | :---: | :---: | :---: | :---: | :---: | --- | --- |
| Bloom Engine | Pass | Pass | Selective | Pass | Pass | Pass | Close radial similarity to Orbital Loom | Two full songs pass; live output |
| Negative Space | Pass | Pass | Sparse | Pass | Pass | Pass | Preserve the dominant void when several roles overlap | Two full songs pass; live output |
| Centrifuge | Pass | Pass | Selective | Pass | Pass | Pass | Side rails can dominate quiet passages | Two full songs pass; live output |
| Constellation Field | Pass | Pass | Sparse | Pass | Pass | Pass | Keep its role-specific node groups legible when events overlap | Two full songs pass; live output |
| Depth Tunnel | Pass | Pass | Flow | Pass | Pass | Pass | Keep deliberate depth travel broad while percussion remains local | Two full songs pass; live output |
| Glass Choir | Pass | Pass | Selective | Pass | Pass | Pass | Keep harmonic movement internal to the fixed shards | Two full songs pass; live output |
| Ink Current | Pass | Pass | Selective | Pass | Pass | Pass | Keep event marks subordinate to the stable current | Two full songs pass; live output |
| Living Mosaic | Pass | Pass | Selective | Pass | Pass | Pass | Keep response inside selected cells and seams | Two full songs pass; live output |
| Lumen Fold | Pass | Pass | Selective | Pass | Pass | Pass | Preserve the calm installation while events use separate sheets, floor pools, cuts, and pins | Two full songs pass; live output |
| Orbital Loom | Pass | Pass | Selective | Pass | Pass | Pass | Needs clearer separation from other radial scenes | Two full songs pass; live output |
| Paper Horizon | Pass | Pass | Sparse | Pass | Pass | Pass | Keep every response on a ridge, tear, star, lantern, or moon detail | Two full songs pass; live output |
| Particle Weave | Pass | Pass | Selective | Pass | Pass | Pass | Keep percussion on separate beads and knots, never the whole textile | Two full songs pass; live output |
| Prism Garden | Pass | Pass | Sparse | Pass | Pass | Pass | Preserve the varied skyline during dense passages | Two full songs pass; live output |
| Pulse Cathedral | Pass | Pass | Selective | Pass | Pass | Pass | Keep outer arches subordinate to the focal rose | Two full songs pass; live output |
| Shadow Architecture | Pass | Pass | Sparse | Pass | Pass | Pass | Keep percussion confined to separate architectural surfaces | Two full songs pass; live output |
| Spectral Ribbons | Pass | Pass | Flow | Pass | Pass | Pass | Keep sustained band contours and transient windows visually separate | Two full songs pass; live output |
| Tidal Grid | Pass | Pass | Selective | Pass | Pass | Pass | Keep foreground grid lines below the horizon subject | Two full songs pass; live output |
| Wire Organism | Pass | Pass | Sparse | Pass | Pass | Pass | Keep its harmonic membrane subtle | Two full songs pass; live output |

## Current priorities

1. Review continuous motion at native display size, not only measured frames,
   and confirm that Depth Tunnel and Spectral Ribbons remain controlled as the two deliberately
   broad flow scenes.
2. Expand the exact rights-cleared real-song set as suitable acoustic, vocal,
   and heavily compressed sources become available. The current two songs and
   five generated profiles pass all 18 scene gates.
3. Exercise launch, track changes, output changes, display sync, and shutdown
   with the same regression discipline as the renderer.
4. Add new scenes only when their composition and motion grammar are clearly
   different from this matrix.
5. Treat radial, filament, depth, vertical, landscape, network, minimal, fluid,
   faceted, and cellular compositions as distinct visual families. Automatic
   direction should not repeat a recent family merely because its audio traits
   are a slightly closer match.

## Motion grammar

- Sparse scenes keep non-transient frame coverage near or below 12 percent and
  reserve most changes for small objects or lines.
- Selective scenes keep non-transient coverage below 18 to 30 percent and
  assign percussion roles to different regions or structures.
- Flow scenes may sustain motion across as much as 45 to 50 percent of the
  frame. No more than one third of the library may use this grammar.

A scene fails the automated quality floor when it exceeds its coverage budget.
It also fails when an isolated kick, snare, or hat changes less than 0.025
percent mean frame luminance. This absolute floor prevents a nearly motionless
baseline from turning an invisible gesture into an impressive response ratio.
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
