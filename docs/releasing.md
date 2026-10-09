# Releasing

These steps are for the maintainer. Housekeeping and CI do not publish anything.
Reuse one build checkout and its caches for the validation loop.

1. Set `VERSION`, `pkgver` in both `packaging/PKGBUILD` and
   `packaging/PKGBUILD.release`, and the changelog to the release version.
   Reset `pkgrel=1` for a new upstream version. Update README and site content;
   add the controls screenshot at `docs/media/controls.png`.
2. Run the checks in [building](building.md), including:

   ```sh
   bash packaging/test-in-arch.sh
   ```

   Confirm `ARCH PACKAGE OK` and the `.pkg.tar.zst` in `dist/`. Check playback,
   Esc, Bluetooth timing and multiple displays on a desktop session.
   Review preset and media credits and unresolved distribution terms.
3. Review the complete diff and release notes, commit the release changes, and
   tag that exact commit. For 0.7.0:

   ```sh
   git tag -a v0.7.0 -m 'Omadrop 0.7.0'
   git push origin HEAD
   git push origin v0.7.0
   ```

4. Verify the release recipe against the published tag, as a regular user on
   Arch. `pacman-contrib` provides `updpkgsums`:

   ```sh
   release_build=$(mktemp -d)
   cp packaging/PKGBUILD.release "$release_build/PKGBUILD"
   cd "$release_build"
   updpkgsums
   makepkg -si
   makepkg --printsrcinfo > .SRCINFO
   ```

   The source URL is
   `https://github.com/btsouth/omadrop/archive/v$pkgver.tar.gz`. Record the tag
   archive's SHA-256 before AUR submission; keep `SKIP` only for pinned Git
   sources. Confirm the installed app reports the intended version. Use this
   build's `.pkg.tar.zst` as the release asset so it matches the public tag.
5. Copy the versioned package to the stable filename for direct downloads,
   then generate checksums beside both packages:

   ```sh
   cp omadrop-0.7.0-1-x86_64.pkg.tar.zst omadrop-x86_64.pkg.tar.zst
   sha256sum omadrop-*.pkg.tar.zst > SHA256SUMS
   sha256sum -c SHA256SUMS
   ```

   Create a GitHub release for `v0.7.0`, using the reviewed changelog entry,
   and attach both `.pkg.tar.zst` files and `SHA256SUMS`. Verify both downloads and
   their checksums. Include the signed-repository install command from the README.
   The [package repository](https://github.com/btsouth/pkgs) imports the versioned
   package, verifies its checksum and signs it within an hour. Confirm it appears
   at https://pkgs.btso.dev/ before announcing repository availability.
6. For the AUR, copy the verified release `PKGBUILD` and `.SRCINFO` into the
   `omadrop` AUR checkout. Review them, commit, and push there. Check the live
   AUR source URL and version. A package-only fix increments `pkgrel`, rather
   than changing the upstream tag.

The package includes dependency licenses and the projectM modification recipe
is in the tagged source. Website deployment is a separate maintainer action;
check release links before deploying updated content.
