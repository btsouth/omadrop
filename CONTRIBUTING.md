# Contributing

Thanks for helping with Omadrop.

## Living worlds

The biggest open project is a contributor kit for living worlds: music-reactive
scenes for Omarchy themes, in the style of Osaka Jade. The kit is not ready
yet. The plan, and the place to say what you would like to work on, is
[Living worlds: contributor kit](https://github.com/btsouth/omadrop/issues/6).

## Bugs and ideas

Open an issue. Include your Omadrop version (`omadrop --version`), Omarchy
version, audio output, GPU and display setup when they matter. For preset
credit corrections or removal, use the removal request template.

## Pull requests

Keep changes focused and follow the existing style. Preserve preset authorship
and include provenance for new assets. Avoid changing audio or visual tuning as
part of unrelated cleanup.

[Architecture](docs/architecture.md) describes the source layout and
[building](docs/building.md) lists dependencies and checks. Run the checks for
the parts you change:

- Launcher: `bash tests/product/product-test`
- Controls: `cd app && bash tests/run-tests.sh`
- Osaka Jade: the CMake tests in `experiments/osaka-live/tests`
- Packaging or installer: `bash packaging/test-in-arch.sh` (needs Docker)
- Website: `cd site && npm ci && npm run build`

For visual or audio changes, attach a short recording with music playing.
Describe the problem, the change and the check results in the pull request.

## Licensing

Omadrop code is [MIT licensed](LICENSE), and contributions are accepted under
the same license. Third-party presets, textures and media keep their own
terms, as listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
