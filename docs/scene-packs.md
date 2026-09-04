# Native scene packs

Scene-pack format 2 describes native fragment shaders without adding them to
Omadrop's official rotation. Validation is read-only. It never installs a pack.

Start from [`examples/scene-pack`](../examples/scene-pack), then run:

```bash
omadrop pack validate path/to/pack
```

Develop a scene in the authoring view:

```bash
omadrop pack author path/to/pack scene-slug
```

The left pane is continuous output and the right pane is the same rendered
frame through the ASCII treatment. Four meters along the bottom show kick,
snare, hat, and energy. The silent deterministic 16-second replay moves through
quiet, groove, dense, and release sections. Press Space to pause, use the arrow
keys to seek by one second, and press Escape to quit. Valid file changes reload
automatically. A rejected edit leaves the last valid shader running and prints
the exact error in the launching terminal.

The current manifest contract is
[`scene-pack-v2.schema.json`](scene-pack-v2.schema.json). A pack declares its
manifest `schemaVersion`, shader `sceneApiVersion`, identity, version, author,
license, and one to 64 scenes. Every scene declares:

- a lowercase slug, display name, and fragment shader;
- kick, snare, and hat roles, plus any groove, harmony, or structure roles;
- its transition anchor, motion grammar, and visual family;
- normalized selection traits used by the director;
- quiet-motion, global-pulse, and frame-time limits;
- scene-level author and license attribution.

Shaders use GLSL 330 core and include `scene-uniforms.glsl`. Scene API 1
provides the music, structure, palette, feedback, artwork, and motion inputs
documented by the bundled `scene-api/1` snapshot. Local `.glsl` includes may
stay inside the pack.

Format 1 packs remain valid and implicitly select scene API 1. Format 2 makes
that dependency explicit with `"sceneApiVersion": 1`. The validator compiles
each pack against its immutable bundled API directory, not Omadrop's changing
internal shader headers. A future Omadrop can therefore add another scene API
without changing the files used by already validated API 1 packs. Unknown
manifest or scene API versions fail with a specific compatibility error.

The validator rejects unknown manifest fields, duplicate scenes or roles, path
escapes, symlinks, oversized files, unsupported samplers, storage or atomic
operations, shader extensions, dynamic or excessive loops, incompatible API
versions, and GLSL compilation errors. It then renders every scene through the
real native path and measures:

- near-still silence;
- sustained quiet-motion coverage against the declared grammar limit;
- visible, distinct kick, snare, and hat responses, each above the same 0.025
  percent mean-frame luminance floor used by official scenes;
- visible beat response above 1.75 times quiet motion when the pack declares a
  groove role;
- global-pulse coverage against the scene's declaration;
- recovery within 167 ms so a hit does not become constant pumping;
- 720p p99 frame time against the scene's declaration.

No pack may declare more than 35 percent global-pulse coverage. A scene can
respond strongly, but a transient cannot justify moving most of the frame.

Validation does not add a scene to the official rotation and does not imply
visual approval.

## Community pack storage

Install a pack only after its static and measured gates pass:

```bash
omadrop pack install path/to/pack
omadrop pack list
omadrop pack remove org.example.pack 1.0.0
```

Community packs are copied atomically into
`$XDG_DATA_HOME/omadrop/community-packs`, or
`~/.local/share/omadrop/community-packs` when `XDG_DATA_HOME` is unset. The
installer rejects links, special files, oversized files, oversized packs, and
duplicate versions. The production director does not scan this directory.
Installation therefore cannot add a community scene to automatic rotation.

Official inclusion remains manual. A scene must also pass the full replay,
motion, silence, transition, ASCII, palette, performance, attribution, and
human visual review gates.
