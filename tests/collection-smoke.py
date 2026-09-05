#!/usr/bin/env python3
"""Load the shipped collection with actual music and isolated presentation state."""
import argparse,csv,hashlib,json,math,os,pathlib,subprocess,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--runtime',type=pathlib.Path,default=ROOT);p.add_argument('--full',action='store_true');p.add_argument('--gallery',type=pathlib.Path);a=p.parse_args()
r=a.runtime.resolve()
manifest=json.loads((ROOT/'experiments/milkdrop-audio-pilot/manifest.json').read_text())
names=(r/'presets/pilot.txt').read_text().splitlines()
assert len(names)==len(set(names))==21
assert names==[x['preset'] for x in manifest['presets']]
for entry in manifest['presets']:
 assert hashlib.sha256((ROOT/entry['original_path']).read_bytes()).hexdigest()==entry['sha256']
 assert (r/'presets/pilot'/entry['preset']).is_file()
out=pathlib.Path(tempfile.mkdtemp(prefix='omadrop-collection-check-'))
print('Evidence:',out,flush=True)
base=os.environ.copy();base.update(OMADROP_TEST_HIDDEN='1',OMADROP_FULLSCREEN='0',OMADROP_DISABLE_MPRIS='1',OMADROP_DISABLE_ART='1',OMADROP_AUDIO_SINK='review',OMADROP_SYNC_MS='0',OMADROP_RANDOM_SEED='42',OMADROP_REVIEW_WIDTH='960',OMADROP_REVIEW_HEIGHT='540',OMADROP_PW_RECORD_COMMAND=str(ROOT/'tests/fixtures/music-input'),XDG_CONFIG_HOME=str(out/'config'),XDG_STATE_HOME=str(out/'state'),XDG_CACHE_HOME=str(out/'cache'),XDG_RUNTIME_DIR=str(out/'runtime'),OMADROP_GPU_DIAGNOSTICS='1')
(out/'runtime').mkdir(mode=0o700)
for key in ['OMADROP_PAIR_ROLE','OMADROP_PAIR_STATE','OMADROP_READY_FILE','OMADROP_START_GATE','OMADROP_PILOT_ORIGINAL']:
 base.pop(key,None)
def run(name,index,seconds,dwell):
 env=base.copy();env.update(OMADROP_START_PRESET=str(index),OMADROP_AUTO_QUIT_MS=str(seconds*1000),OMADROP_REVIEW_DWELL_MS=str(dwell),OMADROP_PILOT_LOG=str(out/(name+'.csv')))
 if a.gallery:
  target=a.gallery/('rotation' if name=='rotation' else str(index+1));target.mkdir(parents=True,exist_ok=True)
  env.update(OMADROP_REVIEW_FRAMES=str(target),OMADROP_REVIEW_INTERVAL_MS='1000')
 if name=='rotation':env.update(OMADROP_REVIEW_WIDTH='1920',OMADROP_REVIEW_HEIGHT='1080',OMADROP_PILOT_TOGGLE_MS='11000')
 result=subprocess.run([str(r/'experiments/projectm-ascii/run-collection.sh')],env=env,capture_output=True,text=True,timeout=seconds+25)
 (out/(name+'.log')).write_text(result.stdout+result.stderr)
 assert result.returncode==0,(name,result.stderr)
 assert not any(s in result.stderr.lower() for s in ['preset load failed','could not load','shader compilation failed']),result.stderr
 rows=list(csv.reader((out/(name+'.csv')).open()));assert rows,name
 data=[[float(x) for x in row] for row in rows]
 assert all(math.isfinite(v) for row in data for v in row)
 assert max(row[3] for row in data)>.01,(name,'no music controls')
 if name=='rotation':assert len({int(row[1]) for row in data})==21,'rotation did not visit every preset'
 print(name,'passed',flush=True)
for i in range(21):run(f'preset-{i+1:02}',i,5 if a.gallery else 2,60000)
if a.full:run('rotation',0,180,1000)
print('All 21 presets loaded with nonzero music controls.',flush=True)
