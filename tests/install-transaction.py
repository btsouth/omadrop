#!/usr/bin/env python3
"""An install failure must restore the previous runtime and keyboard config."""
import os
import pathlib
import subprocess
import tempfile

r = pathlib.Path(__file__).resolve().parents[1]
t = pathlib.Path(tempfile.mkdtemp(prefix='omadrop-install-transaction-'))

# An existing 0.4.0-rc.1 runtime that must survive a failed upgrade.
root = t / 'installed'
root.mkdir()
(root / 'VERSION').write_text('0.3.0\n')
(root / 'sentinel').write_text('keep previous runtime')
(root / 'bin').mkdir()
(root / 'bin/omadrop').write_text('#!/bin/sh\nexit 0\n')
(root / 'bin/omadrop').chmod(0o755)
commands = t / 'bin'
commands.mkdir()
(commands / 'omadrop').symlink_to(root / 'bin/omadrop')

config = t / 'config/hypr'
config.mkdir(parents=True)
bindings = config / 'bindings.lua'
bindings.write_text('-- existing user bindings\n')

# A staged root so the transaction is exercised without a full build.
prebuilt = t / 'prebuilt'
for name in ('omadrop', 'omadrop-milkdrop', 'omadrop-ui', 'omadrop-osaka'):
    path = prebuilt / 'bin' / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text('#!/bin/sh\nexit 0\n')
    path.chmod(0o755)
live = prebuilt / 'experiments/projectm-ascii/projectm-ascii-live'
live.parent.mkdir(parents=True)
live.write_text('#!/bin/sh\nexit 0\n')
live.chmod(0o755)
(prebuilt / 'presets').mkdir()
(prebuilt / 'presets/pilot.txt').write_text(''.join('scene-%02d.milk\n' % i for i in range(1, 22)))
(prebuilt / 'VERSION').write_text('0.5.0-rc.1\n')

fake = t / 'fake'
fake.mkdir()
for name, script in [
    ('omarchy', '#!/bin/sh\nexit 0\n'),
    ('hyprctl', '#!/bin/sh\nif [ "$1" = configerrors ]; then echo "injected binding failure"; fi\n'),
]:
    path = fake / name
    path.write_text(script)
    path.chmod(0o755)

env = os.environ.copy()
env.update(
    OMADROP_INSTALL_ROOT=str(root),
    OMADROP_BIN_DIR=str(commands),
    HOME=str(t / 'home'),
    XDG_DATA_HOME=str(t / 'data'),
    XDG_CONFIG_HOME=str(t / 'config'),
    PATH=str(fake) + ':' + env['PATH'],
)
env.pop('OMADROP_SKIP_HYPR_RELOAD', None)
result = subprocess.run(
    [str(r / 'install.sh'), '--no-deps', '--prebuilt', str(prebuilt)],
    env=env, capture_output=True, text=True, timeout=120,
)
(t / 'install.log').write_text(result.stdout + result.stderr)
assert result.returncode != 0 and 'restored' in result.stderr, result.stderr
assert (root / 'sentinel').read_text() == 'keep previous runtime'
assert (root / 'VERSION').read_text() == '0.3.0\n'
assert bindings.read_text() == '-- existing user bindings\n'
assert (commands / 'omadrop').resolve() == root / 'bin/omadrop'
print('Failed install restored runtime, command and bindings:', t)
