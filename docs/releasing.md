# Release engineering

The 0.4 candidate promotes the revision 6 collection to the normal launcher.
`bin/build-install-runtime` builds the application and installed tools.
`bin/omadrop-check` additionally builds and runs the relevant regression tests.
`bin/omadrop-native-check` retains the older native-engine audit separately.

projectM is fetched at an exact revision with its pinned eval submodule, then
patched in a separate build directory. The source cache must be clean. Generated
presets and binaries are build outputs; the generator checks original hashes.
The original equations and audio mappings remain unchanged from revision 6.

Run `bin/omadrop-check --full` before packaging. Check paired windows, output
switching and live artwork on the intended desktop as well. Raw pacing logs
report p95, p99, maximum, and counts over 33.3/100 ms. Rendering tests establish
function and timing, not artistic approval or end-to-end synchronization.

`bin/omadrop-package --candidate` creates a source archive from a clean commit
without requiring a public release tag. It rebuilds the extracted source into
isolated paths, exercises the installed collection, verifies an upgrade retains
settings, and uninstalls. Stable packages require an exact version tag.

The installer assembles the runtime before replacing the previous version.
Previous runtimes are retained beside the install root as `omadrop.previous.*`.
Settings and cached covers are separate. To roll back, close Omadrop, move the
current installation aside and restore the previous directory to the same path.
Keep the main command symlinks pointing at that path.

Publication checklist:

- Complete candidate validation and inspect the exact release text and demo.
- Resolve the selected presets' and textures' distribution basis; attribution
  notices do not establish individual permissions.
- Keep source revisions, modification recipe and dependency licenses with the
  release. Presets and textures are not licensed under Omadrop's MIT license.
- Replace the preview website label only when that version is actually available.
- Publish only after approval. The current candidate is not a public release.
