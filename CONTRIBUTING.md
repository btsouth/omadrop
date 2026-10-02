# Contributing

Open an issue for bugs or a proposed change. Include your Omadrop version,
Omarchy version, audio output, GPU and display setup when they matter. For
preset credit corrections or removal, use the removal request template.

Keep changes focused. Follow the existing style and keep public descriptions
short. Preserve preset authorship and include provenance for new assets.
Avoid changing audio or visual tuning as part of unrelated cleanup.

See [architecture](docs/architecture.md) for the source layout and
[building](docs/building.md) for dependencies and checks. Run the product,
controller and Rust tests for changes to those components. Run the Arch
container check for build, installer or package changes and the site build for
website changes. Use devbox for heavy checks and an isolated desktop for live
playback checks.

Send a pull request with the problem, change and relevant check results.
Omadrop code is MIT licensed; third-party assets keep their own terms, as
listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
