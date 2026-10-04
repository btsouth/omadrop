# Building and testing

Use an Arch Linux environment. Build dependencies include GCC, Git, Python,
CMake, Ninja, pkgconf, Qt 6 base and declarative, SDL2, GLEW, libpng,
FFTW and json-c. `packaging/PKGBUILD` lists package dependencies in full.
The renderer links to a private patched projectM 4.1.7, with pinned projectm-eval.

## Source install

```sh
./install.sh
```

This installs dependencies, builds all three components and installs for the
current user. `--no-deps` skips package installation; `--no-bindings` skips
Omarchy shortcuts. `--stage /absolute/path` builds a runtime without installing
it. `--prebuilt /absolute/path` installs an already staged runtime.

For a renderer-only rebuild, use `bin/build-milkdrop-runtime`. For the renderer
and installed tools, use `bin/build-install-runtime`. The controls build with:

```sh
mkdir -p app/build
cd app/build
qmake6 ../omadrop-ui.pro
make -j"$(nproc)"
```

The renderer's historical paths under `experiments/` are required build inputs.
Keep `cache/`, `app/build/` and `experiments/osaka-live/build/` for incremental work.

## Arch package

```sh
packaging/makepkg.sh -si
```

The wrapper snapshots the checkout, including local changes, and builds one
package from source. Run it as a regular user; makepkg uses sudo when needed.
It fetches the pinned projectM sources before building.
The package installs `/usr/lib/omadrop`, `/usr/bin/omadrop`, a desktop entry,
an icon and third-party notices. It does not change Hyprland bindings.

## Checks

Run these from the repository root after changes:

```sh
bash -n install.sh uninstall.sh packaging/makepkg.sh packaging/test-in-arch.sh
bash tests/product/product-test
(cd app && bash tests/run-tests.sh)
cmake -S experiments/osaka-live/tests -B experiments/osaka-live/build-tests
cmake --build experiments/osaka-live/build-tests
ctest --test-dir experiments/osaka-live/build-tests --output-on-failure
bash packaging/test-in-arch.sh
(cd site && npm ci && npm run build)
```

Product checks use mocked processes. Controller tests use Qt offscreen and do
not need a desktop. The container check requires Docker; it builds and installs
in `archlinux:latest`, checks libraries and all 21 presets, runs the controller
offscreen, removes the package, and copies the package to `dist/`. Success ends
with `ARCH PACKAGE OK`.

Playback, Bluetooth timing and multiple displays need a real desktop session.
Headless checks cannot establish those.

The website requires Node.js 22.12 or newer, as specified by
its locked Astro dependency. CI runs the fast shell, product, controller and
Osaka checks. [Releasing](releasing.md) covers package publication.
