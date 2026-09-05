#!/usr/bin/env python3
"""An install failure must restore the previous runtime and keyboard config."""
import os,pathlib,subprocess,tempfile
r=pathlib.Path(__file__).resolve().parents[1];t=pathlib.Path(tempfile.mkdtemp(prefix='omadrop-install-transaction-'))
root=t/'installed';root.mkdir();(root/'VERSION').write_text('0.3.0\n');(root/'sentinel').write_text('keep previous runtime')
(root/'bin').mkdir();(root/'bin/omadrop').write_text('#!/bin/sh\nexit 0\n');(root/'bin/omadrop').chmod(0o755)
commands=t/'bin';commands.mkdir();(commands/'omadrop').symlink_to(root/'bin/omadrop')
config=t/'config/hypr';config.mkdir(parents=True);bindings=config/'bindings.lua';bindings.write_text('-- existing user bindings\n')
fake=t/'fake';fake.mkdir()
for name,script in [('omarchy','#!/bin/sh\nexit 0\n'),('hyprctl','#!/bin/sh\nif [ "$1" = configerrors ]; then echo "injected binding failure"; fi\n')]:
 p=fake/name;p.write_text(script);p.chmod(0o755)
env=os.environ.copy();env.update(OMADROP_INSTALL_ROOT=str(root),OMADROP_BIN_DIR=str(commands),XDG_CONFIG_HOME=str(t/'config'),PATH=str(fake)+':'+env['PATH']);env.pop('OMADROP_SKIP_HYPR_RELOAD',None)
result=subprocess.run([str(r/'install.sh'),'--no-deps'],env=env,capture_output=True,text=True,timeout=240)
(t/'install.log').write_text(result.stdout+result.stderr)
assert result.returncode!=0 and 'restored' in result.stderr,result.stderr
assert (root/'sentinel').read_text()=='keep previous runtime'
assert (root/'VERSION').read_text()=='0.3.0\n'
assert bindings.read_text()=='-- existing user bindings\n'
assert (commands/'omadrop').resolve()==root/'bin/omadrop'
print('Failed install restored runtime, command and bindings:',t)
