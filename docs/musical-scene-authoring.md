# Musical scene authoring, interface v1

The engine owns audio analysis, scaling, and time-based smoothing. A scene
owns composition and the assignment of sound roles to visible structures.
Every moving or changing part must have a measured audible cause, followed by
at most a brief settling response. A held input holds its visual state.

## Available inputs

Include `musical-response.glsl` instead of `scene-uniforms.glsl`. This interface
intentionally exposes no time, predicted beat, section, or automatic drive.

| Input | Intended use |
| --- | --- |
| `impactMotion.xyz` | Damped kick, snare, and hat estimates for geometry |
| `kick`, `snare`, `hat` | Immediate attack accents in material or light |
| `musicalBand(0/1/2)` | Smooth low, middle, high frequency levels in 0..1 |
| `musicalDetail(position)` | Smooth spectral detail, position 0..1 |
| `motionScale` | Scale displacements, including reduced-motion policy |
| `albumColor`, palette helpers | Track palette, fixed during a passage |
| `resolution` | Aspect ratio and pixel-correct antialiasing |
| `renderSeconds`, `previousFrame` | Optional elapsed-time feedback only |

These are estimates from mixed audio, not isolated instruments. Do not call a
midrange channel a vocal detector. Do not integrate a held level into ongoing
rotation, travel, noise animation, or color cycling. Feedback must decay with
elapsed seconds, never a fixed factor per frame.

## Starting a scene

A minimal fragment shader is:

```glsl
#version 330 core
in vec2 uv;
out vec4 color;
#include "musical-response.glsl"
void main() {
    vec2 p = (uv - 0.5) * vec2(resolution.x / resolution.y, 1.0);
    float radius = 0.12 + motionScale *
        (0.05 * musicalBand(0) + 0.07 * impactMotion.x);
    float ring = line(length(p) - radius, 0.003);
    vec3 light = palettePrimary(0.2) * ring * (0.25 + kick);
    color = vec4(musicalFinish(light), 1.0);
}
```

This only demonstrates the interface. A finished scene needs distinct visible
roles for all supported audio channels and an authored composition.

1. Add the scene to the existing native registry and shader set, following
   its material and transition definitions.
2. Include the shared musical interface and document each role's visual target.
3. Add its enum to `musicalScenes` in `musical_scenes.h`. This opts it into
   display-rate presentation and the focused preview navigation.
4. Run `native-music-connection-test shaders/native cache/review SLUG` after
   building. It resolves scenes from that shared list. Add the slug to the
   musical contract loop in `bin/omadrop-check`.
5. Inspect real rendered frames and listen to varied music. Automated signal
   tests cannot establish correct instrument identification or visual quality.

This interface is currently for built-in native scenes. Third-party scene pack
API versioning and packaging still need a separate integration pass; existing
packs do not automatically gain this contract.

## Current candidate scenes

These replace the rejected technical-demo art. Stable shader IDs are retained;
none has aesthetic or long-listening acceptance yet.

| Scene | Bass | Snare / middle frequencies | Hats / high frequencies |
| --- | --- | --- | --- |
| Opal Bloom (`constellation-field`) | Sculptural body and warm lower light | Fold opening and seam highlights | Fine iridescent ridges |
| Ember Atlas (`prism-garden`) | Terrain elevation and low glow | Strata and contour emphasis | Fine crest detail |
| Chromatic Pleats (`ink-current`) | Broad lower pleats | Middle seams and folds | Fine foreground fibers |

The shared contract tests role distinction, sustained response, recovery within
half a second, held-input stillness, silence, immunity to inferred rhythm and
scene time, bounded pixels, and GPU cost. Sparse scenes use lower whole-image
response thresholds because their changing structures occupy less screen area.
The original Ink thresholds remain intact. Strong-hit scaling is also covered
by `musical-motion-test`. Existing reduced-motion cue-retention release failures
are not waived by these tests.

Preview: Super+Shift+V starts Opal Bloom. N goes to Ember Atlas, then
Chromatic Pleats, then wraps. P reverses the sequence. No automatic scene changes.
The normal launcher retains its full scene library.

## Previous prototype validation

The measurements below describe the earlier rejected art. See the current
collection plan and batch evidence for the replacement candidates.

All three musical scene contracts pass. Motion unit tests, paired transport,
paired synchronization, and scene-state tests pass. The installed GPU probe
reports `native_renderer=ok`, and all 45 installed runtime files match source.
Hidden installed runs measured Constellation at approximately 6.944 ms per
frame (144 Hz) and Prism at 6.061 ms (165 Hz), with the correct audio clock.
Constellation had an isolated 11.95 ms interval during that test; these short
hidden runs do not establish visible dual-display smoothness. Rendered role
frames were inspected. Evidence is in `cache/musical-scenes/`.

The legacy renderer audit is not green, including its requirements for
predicted beats and anticipation. It must be reconciled with the causal
contract rather than restoring invented movement. The earlier reduced-motion
cue-retention blocker also remains unaddressed. These are listening previews,
not a release acceptance claim.
