# Next session handoff

Updated: 2026-09-04

Implementation checkpoint: `55269930cba1c014a06b914bb7c7995285016bc8`

Branch at handoff: `master`, 97 commits ahead of `origin/master` before this
documentation commit

The implementation run ended at a clean, installed checkpoint. The next
session should finish release-candidate review and validation before adding
features or scenes. Do not assume that local commits have been pushed.

## Product rules

- Make musical attacks, groove, instrumentation, and arrangement visible.
- Keep silence nearly still.
- Do not make complete scenes thump, bounce, flash, or jitter in response to
  ordinary audio. Use local, scene-specific roles.
- Prefer stable, beautiful compositions over constant motion or more scenes.
- Keep automatic scene changes tied to musical structure. `N` skips the scene
  without leaving automatic mode.
- Open with high-resolution cover art, including beneath the ASCII material.
- Persist user preferences and keep both displays synchronized.
- Treat deaf and hard-of-hearing viewers as a product-research audience. Do not
  make accessibility claims until direct testing supports them. Omadrop is not
  being designed as a visualizer for blind users.
- Keep audio analysis local.
- Use only the `btsouth` GitHub account and `btsouth/omadrop` remote. Tyler owns
  pushing, tagging, and publishing unless he explicitly asks otherwise.

## Proven at this checkpoint

- The native library contains 18 distinct scenes.
- All 18 scenes pass the five locked replay profiles, 90 scene-profile
  combinations in total.
- Both complete rights-cleared songs pass the native scene and director gates.
- Silence, sustained low/mid/high response, isolated kick/snare/hat response,
  recovery, role separation, pulse duty, reduced-motion, flash-limit, ASCII,
  and transition behavior have automated coverage.
- Launch, output switching, display hotplug, display synchronization,
  suspend/resume, shutdown, preference persistence, scene-pack isolation,
  packaging, and crash diagnostics have regression coverage.
- The local accessibility-study tools and anonymous scene-review tools work.
  They enable human review but are not evidence that human review happened.
- The current installed build passed the GPU doctor check.
- The latest visual rebuilds are:

  - `5526993` Centrifuge
  - `b711f35` Glass Choir
  - `77b472e` Prism Garden
  - `6b28bab` Wire Organism
  - `9fbbabe` Constellation Field
  - `75b26df` Orbital Loom

  Each rebuild passed the complete synthetic profile set and both approved
  songs. Centrifuge, Glass Choir, Prism Garden, Wire Organism, and
  Constellation Field reported zero broad-pulse frames. Orbital Loom remained
  below 0.5 percent moderate-pulse duty on both real tracks.

## Not yet proven

These are the release-candidate blockers:

1. A new anonymous full-library review has not been generated and scored after
   the latest scene rebuilds.
2. The exact final commit has not completed one fresh
   `bin/omadrop-check --full` run, including its 10-minute soak.
3. The silent visual-legibility study has not been completed by deaf or
   hard-of-hearing participants. Do not turn engineering results into public
   accessibility claims.
4. The final build still needs a short live desktop smoke test for mid-song
   launch, both displays, input focus, output changes, pause, resume, and clean
   shutdown.
5. The public demo film predates the latest scene rebuilds. A new
   rights-cleared film should be recorded only after the visuals are locked.
6. The exact package, tag, release notes, and public release are not complete.
   Packaging does not authorize pushing, tagging, or publishing.

## Next work order

1. Confirm the repository, identity, remote, and build state with the resume
   commands below.
2. Generate a 1920x1080 anonymous motion review of all 18 scenes from one of
   the approved raw replay files:

   ```bash
   mkdir -p cache/tmp
   OMADROP_MOTION_REVIEW_WIDTH=1920 \
   OMADROP_MOTION_REVIEW_HEIGHT=1080 \
   TMPDIR="$PWD/cache/tmp" \
     bin/native-motion-review --anonymous \
       cache/rights-replay/beat-me.f32 \
       cache/final-anonymous-review-2026-09-04 26 18
   ```

   Use the contact sheet and silent motion boards before opening
   `truth/scene-map.tsv`. Fill in `review.tsv`. Score still quality, motion,
   music legibility, constant pulse or jitter, similarity, and ship status.
3. Review every scene at native size. Pay special attention to Depth Tunnel
   and Spectral Ribbons, the two deliberately broad flow scenes. Fix any scene
   rated below 4 out of 5 for still, motion, or music response, or any scene
   marked for constant pulse, jitter, or excessive similarity. Re-run all five
   profiles and both songs for every changed scene.
4. Run the complete gate on the exact final candidate:

   ```bash
   ./bin/omadrop-check --full
   ```

   Keep the resulting `cache/release-check-*` evidence with the candidate.
5. Perform one short live smoke test:

   - Launch during a playing song. Cover art appears first, then reveals the
     visual cleanly.
   - Both displays are fullscreen and synchronized from launch to exit.
   - Keyboard controls work regardless of which display has focus.
   - `N` skips once and remains in automatic direction.
   - ASCII and other preferences survive a restart.
   - Paused or silent audio becomes nearly still.
   - Output switching, hotplug, suspend/resume, and exit recover cleanly.
6. Run the identity-masked study with at least five consenting deaf or
   hard-of-hearing participants before making accessibility claims. Follow
   [accessibility-testing.md](accessibility-testing.md), investigate weak
   scores, and repeat after meaningful mapping changes.
7. After the visuals are locked, record a short rights-cleared release film.
   Confirm that no browser, microphone, notification, or unrelated audio is in
   the capture. Show the strongest three to five scenes with musical
   transitions rather than waiting through a complete automatic performance.
8. Build and inspect the final package. Stop before push, tag, or publication
   unless Tyler explicitly asks for those actions.

## Resume commands

```bash
git status --branch --short
git log -8 --oneline
git remote -v
git config user.name
git config user.email
gh auth status
experiments/projectm-ascii/build.sh
omadrop-doctor --gpu
```

Expected GitHub state at this handoff:

- Remote: `git@github.com:btsouth/omadrop.git`
- Active `gh` account: `btsouth`
- Commit author: `Tyler South <tsouth2@gmail.com>`

Use `cache/` and `TMPDIR="$PWD/cache/tmp"` for large generated review artifacts.
Previous `/tmp` pressure made it a poor location for full native-size reviews.

## Do not repeat completed work

- Do not redesign the core analysis contract, scene registry, director,
  transition system, preference model, multi-display control path, or scene-pack
  system without a specific regression that requires it.
- Do not add scenes to increase the count. The current library already has 18.
- Do not replace human visual judgment with automated scores.
- Do not publish from the next session without explicit authorization.
