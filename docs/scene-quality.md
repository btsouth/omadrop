# Native scene quality matrix

This is the working visual review for the current ten scenes. A scene is not
release-ready until its still, motion, transient roles, quiet behavior,
continuous material, ASCII material, palettes, and transitions all pass.

`Pass` below applies only to the evidence already reviewed. It is not a final
ship decision.

| Scene | Still | Response | Grammar | Color | Gray | Main issue | Next review |
| --- | :---: | :---: | :---: | :---: | :---: | --- | --- |
| Bloom Engine | Pass | Pass | Selective | Pass | Pass | Close radial similarity to Orbital Loom | Full-song motion, ASCII |
| Centrifuge | Pass | Pass | Selective | Pass | Pass | Side rails can dominate quiet passages | Full-song motion, ASCII |
| Constellation Field | Pass | Pass | Sparse | Pass | Pass | Central rings can compete with the network | Full-song motion, ASCII |
| Depth Tunnel | Pass | Pass | Flow | Pass | Tune | Layers lose hierarchy without hue separation | Full-song motion, grayscale |
| Orbital Loom | Pass | Pass | Selective | Pass | Pass | Needs clearer separation from other radial scenes | Full-song motion, ASCII |
| Prism Garden | Tune | Pass | Sparse | Pass | Tune | Repeated columns need more depth and development | Composition, ASCII |
| Pulse Cathedral | Tune | Pass | Selective | Pass | Tune | Bright arches flatten into one layer in grayscale | Composition, grayscale |
| Spectral Ribbons | Tune | Pass | Flow | Pass | Tune | Bands merge and remain visually repetitive | Composition, full-song motion |
| Tidal Grid | Tune | Pass | Selective | Pass | Pass | Horizon, grid, and foreground compete for attention | Composition, ASCII |
| Wire Organism | Tune | Pass | Sparse | Pass | Pass | Strong identity but too little supporting structure | Composition, quiet passages |

## Current priorities

1. Refine Spectral Ribbons without losing its newly separated low, middle, and
   high musical roles.
2. Strengthen the focal hierarchy in Pulse Cathedral, Prism Garden, Tidal Grid,
   and Wire Organism.
3. Review Depth Tunnel and Orbital Loom over full songs to confirm that their
   deliberately broad motion does not become constant pulsing.
4. Fix the four grayscale hierarchy failures, then review every scene in ASCII
   before promoting another scene.
5. Add new scenes only when their composition and motion grammar are clearly
   different from this matrix.

## Motion grammar

- Sparse scenes keep non-transient frame coverage near or below 12 percent and
  reserve most changes for small objects or lines.
- Selective scenes keep non-transient coverage below 18 to 30 percent and
  assign percussion roles to different regions or structures.
- Flow scenes may sustain motion across as much as 42 to 45 percent of the
  frame. No more than one third of the library may use this grammar.

A scene fails the automated quality floor when it exceeds its coverage budget.
Full-frame brightness and scale pulses are not valid substitutes for distinct
kick, snare, and hat gestures. The global-pulse score multiplies changed pixel
coverage by same-direction luminance coherence and has a scene-specific limit.
