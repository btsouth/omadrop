# Arch package

`PKGBUILD` builds from a local source snapshot made by `makepkg.sh`.
`PKGBUILD.release` uses the GitHub tag archive instead. Both build the patched
projectM library, MilkDrop renderer, Osaka Jade and Qt controls from source.

```sh
packaging/makepkg.sh -si
packaging/test-in-arch.sh
```

The package installs one runtime in `/usr/lib/omadrop`, one command in
`/usr/bin`, a desktop entry and icon, and licenses in
`/usr/share/licenses/omadrop`. It does not change Hyprland bindings.

`makepkg.sh` includes local edits in its source archive. projectM and
projectm-eval are pinned Git sources. The sources are prepared before `build()` runs offline. The Docker check builds and installs
in a clean Arch container, runs smoke checks, removes the package, copies it to
`dist/`, and ends with `ARCH PACKAGE OK`.

See [building](../docs/building.md) and [releasing](../docs/releasing.md).
