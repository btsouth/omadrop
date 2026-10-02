from pathlib import Path
import os, subprocess, tempfile, sys
bundle=Path(sys.argv[1]) if len(sys.argv)>1 else Path('/tmp/omadrop-product-candidate/omadrop-0.5.0-preview.1')
for existing in (False, True):
    with tempfile.TemporaryDirectory(prefix='omadrop-bundle-check-') as directory:
        home=Path(directory)
        env=dict(os.environ, HOME=str(home), XDG_DATA_HOME=str(home/'data'), XDG_CONFIG_HOME=str(home/'config'), XDG_STATE_HOME=str(home/'state'), OMADROP_BIN_DIR=str(home/'bin'))
        prior=home/'data/omadrop-screensaver/bin/ttfx-music'
        command=home/'bin/omadrop'
        if existing:
            prior.parent.mkdir(parents=True)
            prior.write_bytes(b'prior-screensaver')
            command.parent.mkdir(parents=True)
            command.write_bytes(b'prior-command')
        settings=home/'config/omadrop/preferences.conf'
        settings.parent.mkdir(parents=True)
        settings.write_text('version=4\ndisplay=single\n')
        for _ in range(2): subprocess.run([str(bundle/'install.sh')],env=env,check=True,stdout=subprocess.DEVNULL)
        assert command.is_symlink()
        owned=(home/'state/omadrop/product-install.list').read_text().splitlines()
        assert len(owned)==len(set(owned)) and len(owned)==(10 if existing else 11)
        assert settings.read_text()=='version=4\ndisplay=single\n'
        if existing:
            command.unlink()
            command.write_bytes(b'newer-command')
        subprocess.run([str(bundle/'uninstall.sh')],env=env,check=True,stdout=subprocess.DEVNULL)
        if existing:
            assert prior.read_bytes()==b'prior-screensaver'
            assert command.read_bytes()==b'newer-command'
            assert (home/'bin/omadrop.before-omadrop').read_bytes()==b'prior-command'
        else:
            assert not prior.exists() and not command.exists()
        assert settings.exists()
print('Bundle: fresh/repeated install, owned manifest, uninstall, existing runtime restore, replacement command and settings preservation passed')
