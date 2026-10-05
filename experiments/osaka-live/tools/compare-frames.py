#!/usr/bin/env python3
"""Compare deterministic Osaka captures. Requires numpy and Pillow."""
import argparse,csv,json,math
from pathlib import Path
import numpy as np
from PIL import Image
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('baseline',type=Path);p.add_argument('candidate',type=Path);p.add_argument('output',type=Path)
a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
base=sorted(a.baseline.glob('*.png'));candidate=sorted(a.candidate.glob('*.png'))
if not base or [x.name for x in base]!=[x.name for x in candidate]:p.error('capture sets differ or are empty')
rows=[]
for b,c in zip(base,candidate):
    x=np.array(Image.open(b).convert('RGB'));y=np.array(Image.open(c).convert('RGB'))
    if x.shape!=y.shape:p.error('image dimensions differ')
    d=np.abs(x.astype(np.int16)-y.astype(np.int16));mse=np.mean(d.astype(np.float64)**2)
    rows.append(dict(frame=b.stem,max=int(d.max()),mean=float(d.mean()),pixels_gt2=float(np.mean(d.max(2)>2)*100),pixels_gt8=float(np.mean(d.max(2)>8)*100),psnr=10*math.log10(255**2/mse) if mse else 'infinity'))
    Image.fromarray(np.concatenate((x,y),axis=1)).save(a.output/(b.stem+'-side-by-side.png'))
    Image.fromarray(np.minimum(d*16,255).astype(np.uint8)).save(a.output/(b.stem+'-diff16.png'))
def series(directory):
    rows=list(csv.DictReader(open(directory/'frames.csv')))
    return rows,sum(float(r['change']) for r in rows[1:])/max(1,len(rows)-1)
b,reactivity_b=series(a.baseline);c,reactivity_c=series(a.candidate)
if [(r['frame'],r['seconds'],r['firework_at'],r['full_show']) for r in b]!=[(r['frame'],r['seconds'],r['firework_at'],r['full_show']) for r in c]:p.error('deterministic clocks or schedules differ')
result=dict(frames=rows,brightness_change_baseline=reactivity_b,brightness_change_candidate=reactivity_c,brightness_change_ratio=reactivity_c/reactivity_b if reactivity_b else None,reactivity_not_reduced=reactivity_c>=reactivity_b,pixel_identical=all(r['max']==0 for r in rows))
(a.output/'metrics.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
