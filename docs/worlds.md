# Build a living world

Living worlds are music-reactive scenes for Omarchy themes. Omadrop includes a
small starter world, a live preview and a readiness check so artists can make
one without rebuilding the renderer.

## Quick start

1. Install Omadrop.
2. Create a world:

   ```sh
   omadrop world new my-theme
   ```

3. Open `my-theme/art.svg` in Inkscape and make it yours. Keep the element IDs
   readable and keep the piece labels on layers that react to music.
4. Preview while you work:

   ```sh
   omadrop world preview my-theme
   ```

5. Run the readiness check:

   ```sh
   omadrop world check my-theme
   ```

6. Open a pull request that adds `worlds/<theme-name>/`.

If you are working in a source clone, create the world directly in the repo:

```sh
omadrop world new my-theme worlds
omadrop world preview worlds/my-theme
omadrop world check worlds/my-theme
```

Name the world after the exact Omarchy theme folder it belongs to. Every
installed world appears in the Omarchy tab, and Omadrop selects yours by default
when that theme is active. For example, a world for the `rose-pine` theme lives
at `worlds/rose-pine/`.

Add a 960x540 `thumbnail.jpg` to the world folder for its card in the controls.
A frame from your recording works well. Without one, the card shows the
world's name.

The world format, supported SVG features and
[piece table](../worlds/README.md) are in the worlds reference. Start with the
labels for `window`, `lantern`, `lamp`, `neon`, `glow` and `wire`, then shape
the rest of the scene around them.

## Style

Osaka Jade is the reference. It stays calm, readable and alive with the music
without becoming frantic. A good world should feel related to its theme while
remaining easy to watch for a long time.

- Keep the scene within the frame budget reported by `omadrop world check`.
- Do not use harsh flashing.
- Make musical movement visible, but keep quiet passages quiet.
- Use original art only and state its license. Contributions use the repo's
  MIT license unless the artwork says otherwise.

## What reviewers check

- `omadrop world check` passes.
- The world name matches the Omarchy theme folder.
- The scene is readable at full size and reacts in a calm, intentional way.
- The artwork has a clear license and no copied third-party assets.
- Edges and the horizon are considered.

Journey mode will later link worlds together and travel between them. Those
edges and the horizon will matter then. No extra metadata is needed yet.
