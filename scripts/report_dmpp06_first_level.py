#!/usr/bin/env python3
"""Strictly audit DMPP06 §2.1/§3.1 finite first-level benchmark output."""
import argparse,csv,hashlib,json,statistics
from pathlib import Path

FIELDS=('component','keys','bucket_bits','capacity_per_bucket','seed','inserted','overflow','hit_checks','miss_checks','build_ns_per_attempt','hit_ns_per_op','miss_ns_per_op','erase_ns_per_op','valid')

def svg(rows,metric,title,path):
    width,height=760,380
    grouped={}
    for row in rows:
        if int(row['bucket_bits'])!=8 or int(row['capacity_per_bucket'])*256 < int(row['keys']): continue
        key=(int(row['keys']),int(row['capacity_per_bucket']))
        grouped.setdefault(key,[]).append(float(row[metric]))
    points=sorted(((k[0],round(statistics.median(v),4)) for k,v in grouped.items()),key=lambda x:x[0])
    if not points:return
    top=max(y for x,y in points)*1.2
    step=550/max(1,len(points)); elems=[]
    for i,(x,y) in enumerate(points):
        px=140+i*step;bar=y/top*225
        elems.append(f'<rect x="{px:.2f}" y="{290-bar:.2f}" width="{step*.73:.2f}" height="{bar:.2f}" fill="#2563eb"/>')
        elems.append(f'<text x="{px:.2f}" y="315" font-size="12" fill="#334155">{x}</text>')
        elems.append(f'<text x="{px:.2f}" y="{279-bar:.2f}" font-size="11" fill="#334155">{y:.2f}</text>')
    path.write_text('<svg xmlns="http://www.w3.org/2000/svg" width="760" height="380" viewBox="0 0 760 380">'
       +'<rect width="760" height="380" fill="white"/>'
       +f'<text x="30" y="35" font-family="Arial" font-size="19" fill="#0f172a">{title}</text>'
       +'<line x1="95" y1="290" x2="735" y2="290" stroke="#64748b"/>'
       +''.join(elems)+'</svg>\n',encoding='utf-8')

def analyze(raw,out):
    out.mkdir(parents=True,exist_ok=True)
    with raw.open(newline='',encoding='utf-8') as f:r=csv.DictReader(f);rows=list(r)
    if tuple(r.fieldnames or ())!=FIELDS:raise ValueError('unexpected author-module CSV schema')
    expected={(bits,n,slack,seed) for bits in (6,8) for n in (4096,8192,16384) for slack in (0,10,25) for seed in (1,2,3,4)}
    seen=set()
    for row in rows:
        if row['component']!='DMPP06_first_filter_2.1_3.1' or int(row['valid'])!=1:raise ValueError('invalid observation')
        bits,n,cap,seed=(int(row[k]) for k in ('bucket_bits','keys','capacity_per_bucket','seed'))
        slack=next((s for s in (0,10,25) if cap==(n*(100+s)+100*(1<<bits)-1)//(100*(1<<bits))),None)
        if slack is None:raise ValueError('unexpected capacity')
        key=(bits,n,slack,seed)
        if key not in expected or key in seen:raise ValueError('duplicate / unsolicited row')
        seen.add(key)
        if int(row['inserted'])+int(row['overflow'])!=n or int(row['hit_checks'])!=int(row['inserted']) or int(row['miss_checks'])!=n:raise ValueError('incomplete operations')
        if not 0<int(row['inserted'])<=n:raise ValueError('empty/nonphysical insertion')
        for m in FIELDS[9:13]:
            val=float(row[m]);
            if not (0<val<1e9):raise ValueError('bad timing '+m)
    if seen!=expected:raise ValueError('missing '+str(len(expected-seen))+' scenarios')
    summary=[]
    for bits in (6,8):
        for n in (4096,8192,16384):
            for s in (0,10,25):
                a=[r for r in rows if (int(r['bucket_bits']),int(r['keys']),int(r['capacity_per_bucket']))==(bits,n,(n*(100+s)+100*(1<<bits)-1)//(100*(1<<bits)))]
                summary.append(dict(bucket_bits=bits,keys=n,slack_pct=s,seeds=4,
                    median_overflow_pct=round(statistics.median(int(r['overflow'])/n*100 for r in a),4),
                    median_hit_ns=round(statistics.median(float(r['hit_ns_per_op']) for r in a),4),
                    median_build_ns=round(statistics.median(float(r['build_ns_per_attempt']) for r in a),4)))
    with (out/'summary.csv').open('w',newline='',encoding='utf-8') as f:
        w=csv.DictWriter(f,fieldnames=list(summary[0]));w.writeheader();w.writerows(summary)
    svg(rows,'hit_ns_per_op','DMPP06 finite first-level hit latency (ns/op)',out/'hit.svg')
    # Show overflow by scenario, selected 8-bit bucket configuration, exact x labels.
    series=[r for r in summary if r['bucket_bits']==8]
    maxv=max((r['median_overflow_pct'] for r in series),default=1) or 1
    bars=[]
    for i,r in enumerate(series):
        x=75+72*i;h=220*r['median_overflow_pct']/maxv
        bars.append(f'<rect x="{x}" y="{285-h:.1f}" width="43" height="{h:.1f}" fill="#d97706"/>')
        bars.append(f'<text x="{x}" y="302" font-size="10" fill="#334155">{r["keys"]}/{r["slack_pct"]}%</text>')
    (out/'overflow.svg').write_text('<svg xmlns="http://www.w3.org/2000/svg" width="800" height="340"><rect width="800" height="340" fill="white"/><text x="25" y="30" font-size="17">DMPP06 finite first-level rejected inserts (%)</text><line x1="60" y1="285" x2="790" y2="285" stroke="#64748b"/>'+''.join(bars)+'</svg>\n',encoding='utf-8')
    evidence={'scope':'DMPP06 LATIN 2006 Section 2.1 finite GF(2^16) 2-independent permutation + Section 3.1 first-level filter ONLY',
     'completed_runs':len(rows),'expected_runs':len(expected),'passes':True,
     'source_restrictions':['finite GF(2^16) experiment, not DMPP06 theorem 5 concentration family', 'explicit unordered_map per bucket, not succinct storage', 'no second/third levels, no complete retrieval guarantees'],
     'raw_sha256':hashlib.sha256(raw.read_bytes()).hexdigest()}
    (out/'audit.json').write_text(json.dumps(evidence,indent=2)+'\n',encoding='utf-8')
    return summary
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('raw',type=Path);p.add_argument('out',type=Path)
    a=p.parse_args();items=analyze(a.raw,a.out);print('PASS: 72/72 runs,',len(items),'summary groups')
