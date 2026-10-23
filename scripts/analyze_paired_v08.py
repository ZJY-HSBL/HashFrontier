#!/usr/bin/env python3
"""Exploratory paired seed bootstrap for three-publication CPU measurements.

No HF-Score or p-values: timing-only pairwise ratios by scenario, without
memory, with seed-level resampling and no cross-scenario ranking.
"""
from __future__ import annotations
import argparse
import csv
import math
import random
import statistics
from collections import defaultdict
from pathlib import Path

METRICS = ('build_ns_per_key','hit_ns_per_op','miss_ns_per_op',
           'insert_ns_per_op','delete_ns_per_op','update_p99_ns')
REFERENCE = 'tpht_flatten'

def analyze(path: Path, output: Path, bootstraps: int = 2000) -> list[dict]:
    with path.open(newline='',encoding='utf-8') as f: rows=list(csv.DictReader(f))
    records={}
    for r in rows:
        if r['status']!='complete' or r['correct']!='true': raise ValueError('incomplete row in observations')
        key=(r['scenario'],int(r['seed']),r['algorithm'])
        if key in records:raise ValueError(f'duplicate {key}')
        records[key]=r
    groups=defaultdict(lambda:defaultdict(list))
    for (scene,seed,alg),r in records.items():
        if alg==REFERENCE:continue
        ref=records.get((scene,seed,REFERENCE))
        if ref is None:continue
        if r['trace_sha256']!=ref['trace_sha256']:raise ValueError(f'UNPAIRED {scene} {seed}')
        for m in METRICS:
            a,b=float(r[m]),float(ref[m]);
            if not (a>0 and b>0 and math.isfinite(a) and math.isfinite(b)):raise ValueError('invalid timing')
            # >1 means target slower than baseline; lower better.
            groups[(scene,alg)][m].append(math.log(a/b))
    out=[]
    for (scene,alg),data in sorted(groups.items()):
        for metric,x in sorted(data.items()):
            n=len(x);rng=random.Random(137+sum(map(ord,scene+alg+metric)))
            means=[sum(x[rng.randrange(n)] for _ in range(n))/n for _ in range(bootstraps)]
            means.sort();lo=means[int(.025*bootstraps)];hi=means[int(.975*bootstraps)]
            out.append({'scenario':scene,'algorithm':alg,'reference':REFERENCE,
                        'metric':metric,'paired_seeds':n,
                        'geomean_target_over_reference':f'{math.exp(statistics.mean(x)):.6f}',
                        'ci95_low':f'{math.exp(lo):.6f}','ci95_high':f'{math.exp(hi):.6f}',
                        'evidence':f'exploratory_{n}_seeds_not_formal_inference'})
    output.parent.mkdir(parents=True,exist_ok=True)
    with output.open('w',newline='',encoding='utf-8') as f:
        fields=['scenario','algorithm','reference','metric','paired_seeds','geomean_target_over_reference','ci95_low','ci95_high','evidence']
        writer=csv.DictWriter(f,fieldnames=fields);writer.writeheader();writer.writerows(out)
    return out

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('observations',type=Path);ap.add_argument('output',type=Path);args=ap.parse_args()
    result=analyze(args.observations,args.output)
    print(f'{len(result)} paired ratios written to {args.output}')
