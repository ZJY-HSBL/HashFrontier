#!/usr/bin/env python3
"""Audit genuine finite DMPP06 §3.2 multi-level tile benchmark and render SVG."""
import argparse
import csv
from collections import defaultdict
from pathlib import Path
import json
import statistics
import hashlib

FIELDS=('insert_ns_attempt','hit_ns_op','miss_ns_op','erase_ns_op')
def audit(rows):
    expected={(bits,tiles,n,seed) for bits in (6,8) for tiles in (16,32,64)
              for n in (4096,8192,16384) for seed in range(1,5)}
    got=set()
    for r in rows:
        key=tuple(int(r[x]) for x in ('bucket_bits','first_tiles','keys','seed'))
        if key not in expected or key in got:raise ValueError(f'unexpected/duplicate row: {key}')
        got.add(key)
        if r['component']!='dmpp06_second_tile' or int(r['levels'])!=4 or int(r['valid'])!=1:
            raise ValueError(f'invalid algorithm or validation: {key}')
        n=int(r['keys']); ok=int(r['inserted']); overflow=int(r['overflow']); erased=int(r['erased'])
        if ok+overflow!=n or ok<=0 or erased != (ok+3)//4 or int(r['hit_ops'])!=10000 or int(r['miss_ops'])!=10000:
            raise ValueError(f'incomplete workload: {key}')
        if int(r['code_capacity']) != (1<<key[0])*(key[1]+key[1]//2+key[1]//4+key[1]//8):
            raise ValueError(f'wrong code space: {key}')
        if any(not (0 < float(r[f]) < 1e9) for f in FIELDS):raise ValueError(f'invalid timing: {key}')
    if got!=expected:raise ValueError(f'missing {len(expected-got)} rows')
    return len(rows)

def svg_chart(stats,metric,ylab):
    subset=[r for r in stats if r['bucket_bits']=='8' and r['keys']=='8192']
    values=[float(r[metric]) for r in subset]
    vmax=max(values)*1.16 if max(values)>0 else 1
    width,height=850,440
    colors=['#2563eb','#16a34a','#c2410c']
    points=[]
    for i,r in enumerate(subset):
        x=120+i*240;y=337-260*float(r[metric])/vmax
        points.append(f'<rect x="{x}" y="{y:.2f}" width="130" height="{337-y:.2f}" fill="{colors[i]}" rx="5"/>')
        points.append(f'<text x="{x+65}" y="{y-10:.2f}" text-anchor="middle" font-size="15">{float(r[metric]):.2f}</text>')
        points.append(f'<text x="{x+65}" y="365" text-anchor="middle" font-size="15">{r["first_tiles"]} tiles</text>')
    for frac in (0,.25,.5,.75,1):
        y=337-frac*260
        points.append(f'<path d="M95 {y:.1f} H805" stroke="#cbd5e1" stroke-width="1"/>')
        points.append(f'<text x="83" y="{y+4:.1f}" text-anchor="end" font-size="13">{vmax*frac:.1f}</text>')
    return ('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 850 440" role="img">'
         '<rect width="850" height="440" fill="white"/>'
         f'<text x="28" y="36" font-family="Arial" font-size="22" font-weight="bold">DMPP06 §3.2 — {ylab}</text>'
         '<text x="28" y="59" font-family="Arial" font-size="13">16-bit finite domain · 256 buckets · 8,192 keys · four cloud seeds (medians)</text>'
         +''.join(points)+'<text x="28" y="416" font-size="12">Measured cloud pilot; tile simulation only. No DMPP06 high-probability or succinct claims.</text></svg>')

def main():
    p=argparse.ArgumentParser();p.add_argument('raw',type=Path);p.add_argument('out',type=Path)
    a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
    with a.raw.open(newline='') as f:rows=list(csv.DictReader(f))
    audit(rows)
    grouped=defaultdict(list)
    for r in rows:grouped[(int(r['bucket_bits']),int(r['first_tiles']),int(r['keys']))].append(r)
    stats=[]
    for (bits,tiles,n),val in sorted(grouped.items()):
        r={'bucket_bits':str(bits),'first_tiles':str(tiles),'keys':str(n),'seeds':str(len(val)),
           'median_overflow_pct':f'{statistics.median(int(x["overflow"])/n*100 for x in val):.6f}'}
        for field in FIELDS:r['median_'+field]=f'{statistics.median(float(x[field]) for x in val):.4f}'
        stats.append(r)
    with (a.out/'summary.csv').open('w',newline='') as f:
        w=csv.DictWriter(f,fieldnames=stats[0].keys());w.writeheader();w.writerows(stats)
    for metric,title,filename in [('median_overflow_pct','Insertion overflow (%)','overflow.svg'),
                                  ('median_hit_ns_op','Successful lookup (ns/op)','hit.svg')]:
        (a.out/filename).write_text(svg_chart(stats,metric,title),encoding='utf8')
    info={'schema':'hashfrontier-dmpp06-second-level/1','real_rows':len(rows),
          'summary_rows':len(stats),'source_sha256_raw':hashlib.sha256(a.raw.read_bytes()).hexdigest(),
          'status':'finite-paper-tile-mechanism-not-full-dmpp06'}
    (a.out/'audit.json').write_text(json.dumps(info,indent=2)+'\n')
    print('DMPP06 Section 3.2 rows',len(rows),'summary groups',len(stats))
if __name__=='__main__':main()
