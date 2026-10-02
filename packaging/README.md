# Packaging

Arch package recipe for the Omadrop one-root install.

- `PKGBUILD` — `omadrop`, built entirely from source into `/usr/lib/omadrop`
  with `/usr/bin/omadrop` symlinked into PATH, one desktop entry and icon, and
  licenses under `/usr/share/licenses/omadrop`.
- `makepkg.sh` — snapshots the checkout (working tree) as the package source
  and runs `makepkg`. projectM v4.1.7 and its `projectm-eval` submodule are
  pinned in the PKGBUILD source list.
- `test-in-arch.sh` — builds and installs the package in a clean
  `archlinux:latest` container via Docker, smoke-checks the installed layout,
  then removes it. Prints `ARCH PACKAGE OK`.

`makepkg.sh` needs no network for the build itself: projectM is fetched by
makepkg from the pinned sources and Rust crates are fetched in `prepare()` with
`cargo fetch --locked`; `build()` then runs offline.

```sh
packaging/makepkg.sh          # makepkg -f in packaging/
packaging/test-in-arch.sh     # full Docker build + install check
```
