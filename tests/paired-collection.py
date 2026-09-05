#!/usr/bin/env python3
"""Exercise real hidden collection windows and control forwarding."""
import csv,os,pathlib,subprocess,tempfile,time
r=pathlib.Path(__file__).resolve().parents[1];out=pathlib.Path(tempfile.mkdtemp(prefix='omadrop-paired-check-'))
base=os.environ.copy();base.update(OMADROP_TEST_HIDDEN='1',OMADROP_FULLSCREEN='0',OMADROP_DISABLE_MPRIS='1',OMADROP_DISABLE_ART='1',OMADROP_AUDIO_SINK='review',OMADROP_SYNC_MS='0',OMADROP_START_PRESET='0',OMADROP_RANDOM_SEED='42',OMADROP_REVIEW_WIDTH='640',OMADROP_REVIEW_HEIGHT='360',OMADROP_PW_RECORD_COMMAND=str(r/'tests/fixtures/music-input'),OMADROP_AUTO_QUIT_MS='14000',OMADROP_REVIEW_DWELL_MS='60000',OMADROP_PAIR_STATE=str(out/'pair'),XDG_CONFIG_HOME=str(out/'config'))
for key in ['OMADROP_PILOT_TOGGLE_MS','OMADROP_PILOT_ORIGINAL','OMADROP_START_GATE','OMADROP_READY_FILE']:base.pop(key,None)
processes=[];logs=[]
try:
 for role in ['leader','follower']:
  env=base.copy();env.update(OMADROP_PAIR_ROLE=role,OMADROP_PILOT_LOG=str(out/(role+'.csv')))
  log=(out/(role+'.log')).open('w');logs.append(log)
  processes.append(subprocess.Popen([str(r/'experiments/projectm-ascii/run-collection.sh')],env=env,stdout=log,stderr=log))
 time.sleep(4)
 (out/'request.tmp').write_text('collection-response\n');(out/'request.tmp').replace(out/'pair.request')
 time.sleep(3)
 (out/'request.tmp').write_text('next\n');(out/'request.tmp').replace(out/'pair.request')
 for proc in processes:assert proc.wait(timeout=25)==0
 for log in logs:log.close()
 last=[]
 for role in ['leader','follower']:
  rows=[[float(x) for x in row] for row in csv.reader((out/(role+'.csv')).open())]
  assert max(row[2] for row in rows)>.9,role
  assert rows[-1][2]<.01,role
  assert len({int(row[1]) for row in rows})>=2,role
  last.append(int(rows[-1][1]))
 assert last[0]==last[1],last
 assert 'paired collection: synchronized' in (out/'follower.log').read_text()
 print('Paired collection audio response and preset request passed:',out)
finally:
 for proc in processes:
  if proc.poll() is None:proc.terminate();proc.wait(timeout=10)
 for log in logs:log.close()
