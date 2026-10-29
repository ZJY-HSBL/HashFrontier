#!/usr/bin/env python3
"""Strict paired evidence audit for Siegel 2004 Lemma 6 finite product graphs."""
import csv, json, math, statistics, sys
from collections import defaultdict
from pathlib import Path

FIELDS = ['mode','base_left','base_right','degree','power','k','seed','left_count','right_count','certified_subsets','evaluations','ns_per_op','graph_payload_bytes','expanded_graph_payload_bytes','table_bytes','checksum','valid']


def audit(path):
    with open(path,newline='',encoding='utf-8') as f:
        reader=csv.DictReader(f)
        if reader.fieldnames!=FIELDS:raise ValueError('invalid CSV schema')
        rows=list(reader)
    if len(rows)!=100:raise ValueError(f'incomplete matrix: expected 100, got {len(rows)}')
    paired=defaultdict(dict)
    for r in rows:
        mode=r['mode'];u=int(r['base_left']);v=int(r['base_right'])
        d=int(r['degree']);power=int(r['power']);k=int(r['k']);seed=int(r['seed'])
        n=int(r['left_count']);R=int(r['right_count']);ops=int(r['evaluations'])
        if mode not in ('factorized','expanded') or u not in (4,6,8) or v!=2*u or d not in (2,3):raise ValueError('wrong graph parameters')
        if power not in (2,3) or (power==3 and u==8) or seed not in (1,2,3,4,5):raise ValueError('unplanned configuration')
        if k!=(4 if power==2 else (3 if u==4 else 2)):raise ValueError('invalid k')
        if n!=u**power or R!=v**power or ops!=150000 or int(r['valid'])!=1:raise ValueError('invalid graph dimension or run')
        expected=sum(math.comb(n,i) for i in range(1,k+1))
        if int(r['certified_subsets'])!=expected:raise ValueError('incomplete finite certificate')
        base_bytes=u*d*2; full_bytes=n*(d**power)*2
        stored=base_bytes if mode=='factorized' else full_bytes
        if int(r['graph_payload_bytes'])!=stored or int(r['expanded_graph_payload_bytes'])!=full_bytes:raise ValueError('incorrect adjacency payload')
        if int(r['table_bytes'])!=8*R:raise ValueError('incorrect independently indexed right table')
        ns=float(r['ns_per_op'])
        if not math.isfinite(ns) or ns<=0 or ns>1e7:raise ValueError('invalid latency')
        int(r['checksum'])
        key=(u,d,power,seed)
        if mode in paired[key]:raise ValueError('duplicate paired observation')
        paired[key][mode]=r
    if len(paired)!=50:raise ValueError('missing experimental configuration')
    for (u,d,power,seed),pair in paired.items():
        if set(pair)!={'factorized','expanded'}:raise ValueError('one-sided paired experiment')
        if pair['factorized']['checksum']!=pair['expanded']['checksum']:
            raise ValueError('factorized and expanded hash outputs diverge')
    return rows,paired


def write_svg(path,title,series,unit):
    width,height=1000,360; y0=285; ymax=max(v for _,v in series)*1.17
    a=[f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="{width}" height="{height}">',
       '<rect width="1000" height="360" fill="white"/>',
       f'<text x="32" y="31" font-family="Arial" font-size="21" font-weight="bold">{title}</text>',
       '<path d="M 80 60 V 285 H 966" stroke="#475467" stroke-width="1.5" fill="none"/>']
    count=len(series)
    for i,(label,value) in enumerate(series):
        x=80+(i+.5)*870/count
        top=y0-(value/ymax)*205
        barwidth=min(56,730/count)
        a.append(f'<rect x="{x-barwidth/2:.1f}" y="{top:.1f}" width="{barwidth:.1f}" height="{y0-top:.1f}" fill="#4777b6"/>')
        a.append(f'<text x="{x:.1f}" y="{top-8:.1f}" text-anchor="middle" font-size="12" font-family="Arial">{value:.2f}</text>')
        a.append(f'<text x="{x:.1f}" y="307" text-anchor="middle" font-size="12" font-family="Arial">{label}</text>')
    a.append(f'<text x="500" y="343" text-anchor="middle" font-size="12" font-family="Arial" fill="#475467">{unit}; payload excludes vector overhead and common independent right-side table</text></svg>')
    Path(path).write_text('\n'.join(a)+'\n',encoding='utf-8')


def report(raw,out):
    rows,pairs=audit(raw)
    out=Path(out);out.mkdir(parents=True,exist_ok=True)
    grouped=defaultdict(list)
    for (u,d,power,seed),p in pairs.items():grouped[(u,d,power)].append(p)
    summary=[]
    for (u,d,power),p in sorted(grouped.items()):
        a=p[0]
        summary.append(dict(base_left=u,degree=d,power=power,seeds=5,
          factorized_median_ns=statistics.median(float(x['factorized']['ns_per_op']) for x in p),
          expanded_median_ns=statistics.median(float(x['expanded']['ns_per_op']) for x in p),
          factorized_graph_bytes=int(a['factorized']['graph_payload_bytes']),
          expanded_graph_bytes=int(a['expanded']['graph_payload_bytes']),
          common_right_table_bytes=int(a['factorized']['table_bytes']),
          certificate_subsets=int(a['factorized']['certified_subsets'])))
    with (out/'summary.csv').open('w',newline='',encoding='utf-8') as f:
        writer=csv.DictWriter(f,fieldnames=list(summary[0]));writer.writeheader();writer.writerows(summary)
    audit_data=dict(valid=True,raw_rows=len(rows),pair_count=len(pairs),summary_configs=len(summary),
      all_checksum_pairs_equal=True,exact_certificate_per_run=True,
      temporary_full_adjacency_used_for_certification=True,
      measured_graph_payload_is_lower_bound=True,
      independent_right_table_required=True,
      siegel_large_scale_explicit_constructor_implemented=False,
      claim_full_siegel_construction=False,
      lemma6_finite_tensor_product=True,
      total_exhaustive_subsets=sum(int(x['factorized']['certified_subsets']) for x in pairs.values()))
    (out/'audit.json').write_text(json.dumps(audit_data,indent=2)+'\n',encoding='utf-8')
    speed=[];memory=[]
    for item in summary:
        if item['degree']==3:
            lab=f"u={item['base_left']} p={item['power']}"
            speed.append((lab,item['factorized_median_ns']/item['expanded_median_ns']))
            memory.append((lab,item['expanded_graph_bytes']/item['factorized_graph_bytes']))
    write_svg(out/'latency.svg','Siegel Lemma 6: factorized / expanded latency ratio',speed,'ratio; lower is better')
    write_svg(out/'adjacency.svg','Siegel Lemma 6: expanded / factorized adjacency payload',memory,'ratio; higher saves more stored edges')
    print(f'PASS 100 raw rows, 50 matched pairs, {len(summary)} summary configs')
    return audit_data

if __name__=='__main__':
    if len(sys.argv)!=3:raise SystemExit('usage: report_siegel_tensor_v026.py RAW.csv OUT_DIRECTORY')
    report(sys.argv[1],sys.argv[2])
