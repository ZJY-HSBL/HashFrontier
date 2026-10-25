#!/usr/bin/env python3
"""Audit *actual* conditional Algorithm 4/5 CSV and draw source-derived SVG."""
import argparse,csv,hashlib,json,statistics
from collections import defaultdict
from pathlib import Path
FIELDS='variant bins bin_size seed keys attempts committed rejected update_ns_per_batch query_ns_per_op planned_swaps prestate_reads physical_bytes external_metadata_min_bytes valid'.split()
def svg(path,rows,metric,title):
    groups=defaultdict(list)
    for r in rows: groups[int(r['bin_size']),int(r['bins'])].append(float(r[metric]))
    ns=[8,16,32,64];bs=[16,32,64];colors=['#006f78','#e4892b','#7351ae']
    med={key:statistics.median(values) for key,values in groups.items()}
    top=max(med.values())*1.15
    out=['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 900 480" width="900" height="480">',
         '<rect width="900" height="480" fill="white"/>',
         f'<text x="40" y="35" font-family="sans-serif" font-size="19">{title}</text>']
    for j in range(5):
        v=top*j/4;y=380-300*j/4
        out+=[f'<line x1="100" y1="{y}" x2="850" y2="{y}" stroke="#ddd"/>',
              f'<text x="92" y="{y+5}" text-anchor="end" font-family="sans-serif" font-size="12">{v:.0f}</text>']
    for ix,B in enumerate(bs):
        pts=[]
        for i,n in enumerate(ns):
            x=110+235*i;y=380-300*med[B,n]/top
            pts.append((x,y))
            out.append(f'<circle cx="{x:.2f}" cy="{y:.2f}" r="5" fill="{colors[ix]}"/>')
        out.append('<polyline fill="none" stroke="'+colors[ix]+'" stroke-width="2.5" points="'+
                   ' '.join(f'{x:.2f},{y:.2f}' for x,y in pts)+'"/>')
        out.append(f'<text x="{115+240*ix}" y="454" fill="{colors[ix]}" font-family="sans-serif">B={B}</text>')
    for i,n in enumerate(ns):
        out.append(f'<text x="{110+235*i}" y="405" text-anchor="middle" font-family="sans-serif">{n} bins</text>')
    out.append('</svg>')
    path.write_text('\n'.join(out)+'\n')
def report(raw,out,seeds=5):
    out=Path(out);out.mkdir(parents=True,exist_ok=True)
    with Path(raw).open(newline='') as f:
        reader=csv.DictReader(f)
        if reader.fieldnames!=FIELDS:raise ValueError('unexpected columns')
        rows=list(reader)
    expected={(n,B,s) for n in [8,16,32,64] for B in [16,32,64] for s in range(1,seeds+1)}
    if len(rows)!=len(expected):raise ValueError('missing or extra observations')
    found=set()
    for r in rows:
        key=tuple(int(r[x]) for x in ('bins','bin_size','seed'))
        if key not in expected or key in found:raise ValueError('duplicate or unexpected case')
        found.add(key)
        if r['variant']!='conditional_alg4_5' or r['valid']!='1':raise ValueError('invalid result')
        if int(r['attempts'])!=12000 or int(r['committed'])+int(r['rejected'])!=12000:raise ValueError('incomplete attempts')
        if int(r['keys'])!=key[0]*key[1]//8:raise ValueError('incorrect size')
        for metric in ('query_ns_per_op','update_ns_per_batch'):
            if float(r[metric])<=0:raise ValueError('nonpositive measurements')
        if int(r['prestate_reads'])<12000:raise ValueError('no prestate reads')
    groups=defaultdict(list)
    for r in rows:groups[int(r['bins']),int(r['bin_size'])].append(r)
    summ=[]
    for (n,B),rs in sorted(groups.items()):
        summ.append(dict(bins=n,bin_size=B,seeds=len(rs),
          median_update_ns_per_batch=round(statistics.median(float(x['update_ns_per_batch']) for x in rs),3),
          median_query_ns_per_op=round(statistics.median(float(x['query_ns_per_op']) for x in rs),3),
          committed=sum(int(x['committed']) for x in rs),rejected=sum(int(x['rejected']) for x in rs)))
    with (out/'summary.csv').open('w',newline='') as f:
        w=csv.DictWriter(f,fieldnames=list(summ[0]));w.writeheader();w.writerows(summ)
    svg(out/'update.svg',rows,'update_ns_per_batch','STOC 2025 | conditional deferred batch updates, ns/batch')
    svg(out/'query.svg',rows,'query_ns_per_op','STOC 2025 | conditional ReadRAM retrieval, ns/op')
    payload={'status':'verified conditional subroutine only','observed_rows':len(rows),'expected_rows':len(expected),
             'raw_sha256':hashlib.sha256(Path(raw).read_bytes()).hexdigest(),
             'full_paper_reproduced':False,'formal_hf_score':False}
    (out/'audit.json').write_text(json.dumps(payload,indent=2)+'\n')
    print('Verified',len(rows),'of',len(expected),'real observations')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--raw',required=True);p.add_argument('--out',required=True);p.add_argument('--seeds',type=int,default=5)
    a=p.parse_args();report(a.raw,a.out,a.seeds)
