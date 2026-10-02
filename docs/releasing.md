# Releasing

These steps are for the maintainer. Housekeeping and CI do not publish anything.
Use the same devbox checkout and caches for the validation loop.

1. Set `VERSION`, `pkgver` in both `packaging/PKGBUILD` and
   `packaging/PKGBUILD.release`, and the changelog to the release version.
   Reset `pkgrel=1` for a new upstream version. Update README and site content;
   add the controls screenshot at `docs/media/controls.png`.
2. Run the checks in [building](building.md), including:

   ```sh
   bash packaging/test-in-arch.sh
   ```

   Confirm `ARCH PACKAGE OK` and the `.pkg.tar.zst` in `dist/`. Check playback,
   Esc, Bluetooth timing and multiple displays in an isolated desktop session.
   Review preset and media credits and unresolved distribution terms.
3. Review the complete diff and release notes, commit the release changes, and
   tag that exact commit. For 0.5.0:

   ```sh
   git tag -a v0.5.0 -m 'Omadrop 0.5.0'
   git push origin HEAD
   git push origin v0.5.0
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
5. Generate checksums beside the package:

   ```sh
   sha256sum omadrop-*.pkg.tar.zst > SHA256SUMS
   sha256sum -c SHA256SUMS
   ```

   Create a GitHub release for `v0.5.0`, using the reviewed changelog entry,
   and attach the `.pkg.tar.zst` and `SHA256SUMS`. Verify both downloads and
   their checksums. Include the install command `sudo pacman -U omadrop-*.pkg.tar.zst`.
6. For the AUR, copy the verified release `PKGBUILD` and `.SRCINFO` into the
   `omadrop` AUR checkout. Review them, commit, and push there. Check the live
   AUR source URL and version. A package-only fix increments `pkgrel`, rather
   than changing the upstream tag.

The package includes dependency licenses and the projectM modification recipe
is in the tagged source. Website deployment is a separate maintainer action;
check release links before deploying updated content.
