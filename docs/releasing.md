# Releasing

These steps are for the maintainer. Only the release workflow publishes
anything, from a pushed tag or a manual rebuild.
Reuse one build checkout and its caches for the validation loop.

1. Set `VERSION`, `pkgver` in both `packaging/PKGBUILD` and
   `packaging/PKGBUILD.release`, and the changelog to the release version.
   Reset `pkgrel=1` for a new upstream version. Update README and site content;
   add the controls screenshot at `docs/media/controls.png`.
2. Run the checks in [building](building.md), including:

   ```sh
   bash packaging/test-in-arch.sh
   ```

   Confirm `ARCH PACKAGE OK`. Check playback, Esc, Bluetooth timing and
   multiple displays on a desktop session.
   Review preset and media credits and unresolved distribution terms.
3. Review the complete diff and release notes, commit the release changes, and
   tag that exact commit. For 0.7.0:

   ```sh
   git tag -a v0.7.0 -m 'Omadrop 0.7.0'
   git push origin HEAD
   git push origin v0.7.0
   ```

4. Pushing the tag runs the [release workflow](../.github/workflows/release.yml).
   It builds the package from the tag archive in a clean container synced to
   Omarchy's stable mirror, the oldest Qt users have, and runs the same checks
   as `test-in-arch.sh`. It then attests the package and publishes the release
   with the changelog entry, the versioned package, a copy under the stable
   name `omadrop-x86_64.pkg.tar.zst` and `SHA256SUMS`. Review the release notes
   on GitHub afterwards and edit them if needed.

   Do not attach a package built on a workstation. One built on Omarchy edge
   links against a newer Qt and fails to start on stable, and the
   [package repository](https://github.com/btsouth/pkgs) only accepts packages
   attested by this workflow. It imports the release, verifies and signs it
   within an hour. Confirm it appears at https://pkgs.btso.dev/ before
   announcing repository availability.
5. A package-only fix keeps the tag. Increment `pkgrel` in both PKGBUILDs on
   master, then rebuild the existing release:

   ```sh
   gh workflow run release.yml -R btsouth/omadrop -f tag=v0.7.0
   ```

   The workflow replaces the package files on that release and the package
   repository picks up the new `pkgrel`.
6. For the AUR, prepare the recipe as a regular user on Arch.
   `pacman-contrib` provides `updpkgsums`:

   ```sh
   release_build=$(mktemp -d)
   cp packaging/PKGBUILD.release "$release_build/PKGBUILD"
   cd "$release_build"
   updpkgsums
   makepkg --printsrcinfo > .SRCINFO
   ```

   The source URL is
   `https://github.com/btsouth/omadrop/archive/v$pkgver.tar.gz`. Keep `SKIP`
   only for pinned Git sources. Copy `PKGBUILD` and `.SRCINFO` into the
   `omadrop` AUR checkout, review them, commit and push there. Check the live
   AUR source URL and version.

The package includes dependency licenses and the projectM modification recipe
is in the tagged source. Website deployment is a separate maintainer action;
check release links before deploying updated content.
