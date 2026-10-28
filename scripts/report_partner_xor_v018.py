#!/usr/bin/env python3
"""Strict audit and SVG rendering for finite XOR-in-partner-RAM control runs.

This is a NON-DMPP06 diagnostic experiment. Never claim a Partner Hashing score.
"""
import argparse
import csv
import json
import math
import statistics
from collections import defaultdict
from pathlib import Path

REQUIRED={'bins','bin_size','ram_words','seed','keys_built','build_ns_per_key',
          'hit_ns_per_query','update_ns_per_op','delete_ns_per_op',
          'encoded_array_bytes','external_metadata_lower_bound_bytes',
          'successful_rebuilds','peel_attempts','placement_attempts','valid'}
METRICS=('build_ns_per_key','hit_ns_per_query','update_ns_per_op','delete_ns_per_op')

def analyze(raw:Path,out:Path,seed_count:int=3):
    if seed_count<1:raise ValueError('seed_count must be positive')
    with raw.open(newline='',encoding='utf8') as f:
        reader=csv.DictReader(f)
        if not reader.fieldnames or set(reader.fieldnames)!=REQUIRED:
            raise ValueError('missing or unexpected CSV columns')
        rows=list(reader)
    expected={(b,B,s) for b in (64,128,256) for B in (32,64)
              for s in range(1,seed_count+1)}
    if len(rows)!=len(expected):raise ValueError('incomplete experiment matrix')
    seen=set()
    by_group=defaultdict(list)
    for r in rows:
        b,B,s=(int(r[k]) for k in ('bins','bin_size','seed'))
        if (b,B,s) not in expected or (b,B,s) in seen:raise ValueError('bad/duplicate configuration')
        seen.add((b,B,s))
        for k in METRICS:
            v=float(r[k]);
            if not math.isfinite(v) or v<=0:raise ValueError('invalid timings')
        for k in ('ram_words','keys_built','encoded_array_bytes','external_metadata_lower_bound_bytes',
                  'successful_rebuilds','peel_attempts','placement_attempts'):
            if int(r[k])<0:raise ValueError('negative count')
        if int(r['ram_words'])!=b*B//4 or int(r['encoded_array_bytes'])!=b*B*8:
            raise ValueError('invalid RAM capacity/bytes')
        if int(r['valid'])!=1 or int(r['keys_built'])!=min(120,b*B//40):
            raise ValueError('incorrect or incomplete trial')
        if int(r['external_metadata_lower_bound_bytes'])==0:
            raise ValueError('hidden external state')
        if int(r['successful_rebuilds'])<int(r['keys_built']):
            raise ValueError('missing rebuilds')
        by_group[(b,B)].append(r)
    out.mkdir(parents=True,exist_ok=True)
    summary=[]
    for (b,B),arr in sorted(by_group.items()):
        item={'bins':b,'bin_size':B,'ram_words':b*B//4,'seeds':len(arr)}
        for metric in METRICS:
            item[metric+'_median']=round(statistics.median(float(x[metric]) for x in arr),3)
        item['external_metadata_lower_bound_bytes']=max(int(x['external_metadata_lower_bound_bytes']) for x in arr)
        summary.append(item)
    with (out/'summary.csv').open('w',newline='',encoding='utf8') as f:
        w=csv.DictWriter(f,fieldnames=summary[0].keys());w.writeheader();w.writerows(summary)
    audit={'label':'NON-DMPP06 control / NOT complete STOC Partner Hashing reproduction',
           'total_rows':len(rows),'expected_rows':len(expected),'complete':True,
           'seed_count':seed_count,'external_index_included':True,
           'dynamic_update_method':'whole-layout 3-hash XOR peel and RAM batch rewrite'}
    (out/'audit.json').write_text(json.dumps(audit,indent=2)+'\n',encoding='utf8')
    for metric,title,name in [('hit_ns_per_query_median','Successful retrieval: finite XOR encoded RAM', 'retrieval.svg'),
                               ('update_ns_per_op_median','Update: whole-layout rebuild control','update.svg')]:
        svg=svg_chart(summary,metric,title)
        (out/name).write_text(svg,encoding='utf8')
    return summary

def svg_chart(summary,metric,title):
    w,h=920,450;left=94;right=885;top=85;bottom=370
    values=[float(r[metric]) for r in summary]
    mx=max(values)*1.15
    def xcoord(n):return left+(math.log2(n)-math.log2(512))/(math.log2(4096)-math.log2(512))*(right-left)
    def ycoord(y):return bottom-(y/mx)*(bottom-top)
    lines=[f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">',
           '<rect width="920" height="450" fill="#fff"/>',
           f'<text x="50" y="42" font-size="21" font-family="Arial" font-weight="bold">{title}</text>',
           '<text x="50" y="62" font-size="13" fill="#555" font-family="Arial">Actual temporary Linux / GitHub results; NOT DMPP06 and not formal cross-paper ranking</text>',
           f'<path d="M{left},{top}V{bottom}H{right}" stroke="#333" stroke-width="1.5" fill="none"/>']
    for k in range(5):
        val=mx*k/4;y=ycoord(val)
        lines+=[f'<path d="M{left},{y:.1f}H{right}" stroke="#ddd"/>',
                f'<text x="{left-8}" y="{y+4:.1f}" text-anchor="end" font-size="12" font-family="Arial">{val:.0f}</text>']
    for n in (512,1024,2048,4096):
        x=xcoord(n)
        lines.append(f'<text x="{x:.1f}" y="{bottom+21}" text-anchor="middle" font-size="12" font-family="Arial">{n}</text>')
    for B,color in ((32,'#1c629b'),(64,'#c45623')):
        grp=sorted((r for r in summary if r['bin_size']==B),key=lambda a:a['ram_words'])
        pts=' '.join(f'{xcoord(r["ram_words"]):.1f},{ycoord(float(r[metric])):.1f}' for r in grp)
        lines.append(f'<polyline points="{pts}" fill="none" stroke="{color}" stroke-width="2.5"/>')
        for r in grp:lines.append(f'<circle cx="{xcoord(r["ram_words"]):.1f}" cy="{ycoord(float(r[metric])):.1f}" r="5" fill="{color}"/>')
        lines.append(f'<text x="{left+(B==64)*180}" y="420" fill="{color}" font-size="14" font-family="Arial">B={B}</text>')
    lines.append('<text x="460" y="398" text-anchor="middle" font-size="14" font-family="Arial">Encoded RAM words</text>')
    lines.append('</svg>')
    return '\n'.join(lines)+'\n'

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--raw',required=True,type=Path)
    p.add_argument('--out',required=True,type=Path);p.add_argument('--seeds',type=int,default=3)
    args=p.parse_args();summary=analyze(args.raw,args.out,args.seeds)
    print(f'PASS {len(summary)} cohorts, {len(summary)*args.seeds} complete runs')
