# Native scene packs

Scene-pack format 1 describes native fragment shaders without adding them to
Omadrop's official rotation. Validation is read-only. It never installs a pack.

Start from [`examples/scene-pack`](../examples/scene-pack), then run:

```bash
omadrop pack validate path/to/pack
```

The manifest contract is
[`scene-pack-v1.schema.json`](scene-pack-v1.schema.json). A pack declares its
identity, version, author, license, and one to 64 scenes. Every scene declares:

- a lowercase slug, display name, and fragment shader;
- kick, snare, and hat roles, plus any groove, harmony, or structure roles;
- its transition anchor, motion grammar, and visual family;
- normalized selection traits used by the director;
- quiet-motion, global-pulse, and frame-time limits;
- scene-level author and license attribution.

Shaders use GLSL 330 core and include `scene-uniforms.glsl`. This provides the
same music, structure, palette, feedback, artwork, and motion inputs as official
scenes. Local `.glsl` includes may stay inside the pack.

The validator rejects unknown manifest fields, duplicate scenes or roles, path
escapes, symlinks, oversized files, unsupported samplers, storage or atomic
operations, shader extensions, dynamic or excessive loops, incompatible API
versions, and GLSL compilation errors. It then renders every scene through the
real native path and measures:

- near-still silence;
- sustained quiet-motion coverage against the declared grammar limit;
- visible, distinct kick, snare, and hat responses;
- global-pulse coverage against the scene's declaration;
- recovery within 167 ms so a hit does not become constant pumping;
- 720p p99 frame time against the scene's declaration.

Validation does not add a scene to the official rotation and does not imply
visual approval.

Official inclusion remains manual. A scene must also pass the full replay,
motion, silence, transition, ASCII, palette, performance, attribution, and
human visual review gates.
