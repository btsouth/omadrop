# Native scene quality matrix

This is the working visual review for the current ten scenes. A scene is not
release-ready until its still, motion, transient roles, quiet behavior,
continuous material, ASCII material, palettes, and transitions all pass.

`Pass` below applies only to the evidence already reviewed. It is not a final
ship decision.

| Scene | Still | Response | Grammar | Color | Gray | ASCII | Main issue | Next review |
| --- | :---: | :---: | :---: | :---: | :---: | :---: | --- | --- |
| Bloom Engine | Pass | Pass | Selective | Pass | Pass | Pass | Close radial similarity to Orbital Loom | Full-song motion |
| Centrifuge | Pass | Pass | Selective | Pass | Pass | Pass | Side rails can dominate quiet passages | Full-song motion |
| Constellation Field | Pass | Pass | Sparse | Pass | Pass | Pass | Central rings can compete with the network | Full-song motion |
| Depth Tunnel | Pass | Pass | Flow | Pass | Pass | Pass | Preserve the stable aperture during dense peaks | Full-song motion |
| Orbital Loom | Pass | Pass | Selective | Pass | Pass | Pass | Needs clearer separation from other radial scenes | Full-song motion |
| Prism Garden | Pass | Pass | Sparse | Pass | Pass | Pass | Preserve the varied skyline during dense passages | Full-song motion |
| Pulse Cathedral | Pass | Pass | Selective | Pass | Pass | Pass | Keep outer arches subordinate to the focal rose | Full-song motion |
| Spectral Ribbons | Pass | Pass | Flow | Pass | Pass | Pass | Keep high-frequency folds from becoming visual noise | Full-song motion |
| Tidal Grid | Pass | Pass | Selective | Pass | Pass | Pass | Keep foreground grid lines below the horizon subject | Full-song motion |
| Wire Organism | Pass | Pass | Sparse | Pass | Pass | Pass | Keep its harmonic membrane subtle | Full-song motion, quiet passages |

## Current priorities

1. Review all ten scenes over full songs, including quiet, dense, and sustained
   passages, before promoting another scene.
2. Confirm that Depth Tunnel and Spectral Ribbons remain controlled as the two
   deliberately broad flow scenes.
3. Exercise launch, track changes, output changes, display sync, and shutdown
   with the same regression discipline as the renderer.
4. Add new scenes only when their composition and motion grammar are clearly
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
