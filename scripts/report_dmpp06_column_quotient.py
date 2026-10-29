#!/usr/bin/env python3
"""Strict finite DMPP06 §2.2 measurement audit and SVG generation."""
import argparse,csv,json,statistics,hashlib
from collections import defaultdict
from pathlib import Path

def report(inp:Path,out:Path,seeds:int=10):
    with inp.open(newline='') as f:
        rows=list(csv.DictReader(f))
    expected={(b,n,d,s) for b in (10,11,12)
              for n in (512,1024,2048) if n<=2**b
              for d in ('contiguous','uniform') for s in range(1,seeds+1)}
    seen=set()
    for row in rows:
        k=(int(row['bucket_bits']),int(row['keys']),
           row['distribution'],int(row['seed']))
        if k not in expected or k in seen:
            raise ValueError(f'unexpected or duplicate {k}')
        seen.add(k)
        if (row['family']!='row_shift_column_affine'
            or int(row['buckets'])!=2**k[0]
            or int(row['correct'])!=1 or int(row['metadata_bytes'])!=1024
            or not(0<=int(row['colliding_keys'])<=k[1])
            or not(1<=int(row['max_bucket'])<=k[1])
            or any(float(row[c])<0 for c in ('forward_ns_per_op','inverse_ns_per_op'))):
            raise ValueError(f'bad measurement {k}')
    if seen!=expected:
        raise ValueError(f'missing {len(expected-seen)} cases')
    groups=defaultdict(list)
    for r in rows:
        groups[(int(r['bucket_bits']),int(r['keys']),r['distribution'])].append(r)
    out.mkdir(parents=True,exist_ok=True)
    summary=[]
    for k,values in sorted(groups.items()):
        summary.append(dict(bucket_bits=k[0],keys=k[1],distribution=k[2],
            seeds=len(values),
            median_colliding_keys=statistics.median(int(x['colliding_keys']) for x in values),
            median_max_bucket=statistics.median(int(x['max_bucket']) for x in values),
            median_forward_ns=round(statistics.median(float(x['forward_ns_per_op']) for x in values),3),
            median_inverse_ns=round(statistics.median(float(x['inverse_ns_per_op']) for x in values),3)))
    with (out/'summary.csv').open('w',newline='') as f:
        w=csv.DictWriter(f,fieldnames=list(summary[0]))
        w.writeheader();w.writerows(summary)
    audit=dict(input_rows=len(rows),expected_rows=len(expected),all_correct=True,
               configuration_count=len(summary),
               raw_sha256=hashlib.sha256(inp.read_bytes()).hexdigest(),
               scope='DMPP06 finite shifts and column permutations; NOT Siegel / Theorem 5')
    (out/'audit.json').write_text(json.dumps(audit,indent=2)+'\n')
    focused=[r for r in summary if r['keys']==1024 and r['distribution']=='contiguous']
    maxv=max(float(r['median_colliding_keys']) for r in focused)+1
    svg=['<svg xmlns="http://www.w3.org/2000/svg" width="760" height="430" viewBox="0 0 760 430">',
         '<rect width="760" height="430" fill="#fff"/>',
         '<text x="45" y="42" font-size="22" font-family="sans-serif">DMPP06 finite column quotient hashing</text>',
         '<text x="45" y="68" font-size="14" fill="#555" font-family="sans-serif">Contiguous 1024-key input, median of 10 seeds; colliding keys</text>']
    for i,r in enumerate(focused):
        y=112+i*86; val=float(r['median_colliding_keys'])
        width=int(485*val/maxv)
        svg.extend([f'<text x="55" y="{y+24}" font-size="16" font-family="sans-serif">{2**r["bucket_bits"]} buckets</text>',
                    f'<rect x="220" y="{y}" width="{width}" height="40" rx="5" fill="#295c8e"/>',
                    f'<text x="{230+width}" y="{y+26}" font-size="15" fill="#222" font-family="sans-serif">{val:g}</text>'])
    svg.append('<text x="45" y="400" font-size="12" fill="#666" font-family="sans-serif">Finite local/VM tests only; NOT a high-probability theorem claim.</text></svg>')
    (out/'collisions.svg').write_text('\n'.join(svg)+'\n')
    print(json.dumps(audit))
    return summary

if __name__=='__main__':
    p=argparse.ArgumentParser()
    p.add_argument('raw',type=Path)
    p.add_argument('out',type=Path)
    p.add_argument('--seeds',type=int,default=10)
    a=p.parse_args()
    report(a.raw,a.out,a.seeds)
