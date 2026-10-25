#!/usr/bin/env python3
"""Strictly audit real Section-3.2 oracle-backed pilot rows; render SVG directly."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import statistics

BINS=(8,16,32,64)
BS=(16,32,64)
FIELDS=('hit_ns_op','miss_ns_op','ram_write_ns_attempt','delete_ns_attempt','insert_ns_attempt')

def load_rows(path, seeds):
    with open(path,newline='',encoding='utf-8') as f: rows=list(csv.DictReader(f))
    expected={(b,B,s) for b in BINS for B in BS for s in range(1,seeds+1)}
    seen=set()
    if len(rows)!=len(expected):raise ValueError(f'row count {len(rows)} != {len(expected)}')
    for r in rows:
        key=(int(r['bins']),int(r['bin_size']),int(r['seed']))
        if key not in expected or key in seen:raise ValueError(f'unexpected or duplicate configuration {key}')
        seen.add(key)
        b,B,s=key
        if r['component']!='partner_section3_2_oracle':raise ValueError('wrong component')
        if int(r['capacity'])!=b*B or int(r['initial_keys'])!=b*B*3//4:raise ValueError('invalid initial state')
        if (r['valid']!='1' or int(r['checksum'])!=2000 or
             int(r['write_attempts'])!=400 or int(r['delete_attempts'])!=240 or
             int(r['insert_successes'])!=int(r['delete_successes']) or
             int(r['write_successes'])>400 or int(r['delete_successes'])>240):
            raise ValueError(f'incorrect or incomplete benchmark execution {key}')
        for field in FIELDS:
            v=float(r[field])
            if not (v>=0 and v<float('inf')):raise ValueError(f'bad timing {key} {field}')
    if seen!=expected:raise ValueError('missing configurations')
    return rows

def svg_chart(rows, metric, path):
    data=[r for r in rows if int(r['bin_size'])==32]
    vals=[float(r[metric]) for r in data]
    maxy=max(vals)*1.08 if vals else 1
    W,H=840,470; left,right,top,bottom=93,30,58,77
    inner_w=W-left-right; inner_h=H-top-bottom
    y=lambda n: top+inner_h*(1-n/maxy)
    x=lambda b:left+(BINS.index(b)/(len(BINS)-1))*inner_w
    groups=[]
    for b in BINS:
        sample=[float(r[metric]) for r in data if int(r['bins'])==b]
        groups.append((b,statistics.median(sample),min(sample),max(sample)))
    def t(v):return f'{v:.1f}'
    marks=[]
    for i in range(5):
        value=maxy*i/4
        yy=y(value)
        marks.append(f'<line x1="{left}" x2="{W-right}" y1="{yy:.2f}" y2="{yy:.2f}" stroke="#dce6f0"/>')
        marks.append(f'<text x="{left-13}" y="{yy+4:.2f}" text-anchor="end" fill="#53657b" font-size="14">{t(value)}</text>')
    for b,med,low,high in groups:
        xx=x(b);marks.append(f'<line x1="{xx:.2f}" x2="{xx:.2f}" y1="{y(low):.2f}" y2="{y(high):.2f}" stroke="#0072bc" stroke-width="2"/>')
        marks.append(f'<circle cx="{xx:.2f}" cy="{y(med):.2f}" r="6" fill="#0072bc"/>')
        marks.append(f'<text x="{xx:.2f}" y="{H-bottom+29}" text-anchor="middle" font-size="15" fill="#243e55">{b}</text>')
    points=' '.join(f'{x(b):.1f},{y(med):.1f}' for b,med,_,_ in groups)
    title=metric.replace('_',' ').replace('ns op','ns/op')
    body='\n'.join(marks)
    svg=f'''<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}"><rect width="100%" height="100%" fill="#ffffff"/><text x="{left}" y="31" font-family="DejaVu Sans,Arial" font-weight="bold" font-size="20" fill="#162e44">{title}</text><text x="{left}" y="51" font-family="DejaVu Sans,Arial" font-size="12" fill="#53657b">Measured benchmark, B=32, 5 seeds; min–max whiskers and median</text>{body}<polyline points="{points}" fill="none" stroke="#0072bc" stroke-width="3"/><text x="{left+inner_w/2}" y="{H-22}" text-anchor="middle" font-family="DejaVu Sans,Arial" font-size="15" fill="#53657b">Number of bins</text><text transform="translate(22,{top+inner_h/2}) rotate(-90)" text-anchor="middle" font-family="DejaVu Sans,Arial" font-size="14" fill="#53657b">ns / attempted operation</text></svg>'''
    path.write_text(svg,encoding='utf-8')

def report(raw,out,figures,seeds,env=None):
    rows=load_rows(raw,seeds)
    out.mkdir(parents=True,exist_ok=True);figures.mkdir(parents=True,exist_ok=True)
    result=[]
    for bins in BINS:
        for B in BS:
            rr=[r for r in rows if int(r['bins'])==bins and int(r['bin_size'])==B]
            item={'bins':bins,'bin_size':B,'capacity':bins*B,'seeds':len(rr)}
            for m in FIELDS:item[m+'_median']=round(statistics.median(float(r[m]) for r in rr),4)
            item['write_successes']=sum(int(r['write_successes']) for r in rr)
            item['delete_successes']=sum(int(r['delete_successes']) for r in rr)
            item['delete_attempts']=sum(int(r['delete_attempts']) for r in rr)
            result.append(item)
    with (out/'summary.csv').open('w',newline='',encoding='utf-8') as f:
        w=csv.DictWriter(f,fieldnames=list(result[0]));w.writeheader();w.writerows(result)
    for met in ('hit_ns_op','ram_write_ns_attempt','delete_ns_attempt'):
        svg_chart(rows,met,figures/(met+'.svg'))
    info={'research_status':'partial_section_3_2_oracle_not_complete_partner_hashing',
          'measurement_context':'finite-cloud-pilot_not_paper_theorem_or_HFScore',
          'operation_counts':{'rows':len(rows),'writes':sum(int(r['write_attempts']) for r in rows),
             'completed_writes':sum(int(r['write_successes']) for r in rows),
             'deletes':sum(int(r['delete_attempts']) for r in rows),
             'completed_deletes':sum(int(r['delete_successes']) for r in rows)},
          'sha256_raw':hashlib.sha256(raw.read_bytes()).hexdigest(),
          'source_reference':'https://arxiv.org/pdf/2503.13628',
          'known_deviations':['external retrieval unordered_map and per-bin counts',
              'external random masks','balanced preconditioned inputs',
              'full-vector rollback on deletion (O(n) worst case)',
              'bounded self-loop rejection','Algorithms 4 and 5 not implemented',
              'Section 4 and 5 not implemented']}
    if env and env.exists():info['environment_text']=env.read_text(encoding='utf-8')
    (out/'audit.json').write_text(json.dumps(info,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(json.dumps({'validated_rows':len(rows),'writes':info['operation_counts']['writes'],
                      'deletes':info['operation_counts']['deletes']},ensure_ascii=False))
    return result,info

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--raw',type=Path,required=True)
    ap.add_argument('--out',type=Path,required=True);ap.add_argument('--figures',type=Path,required=True)
    ap.add_argument('--seeds',type=int,default=5);ap.add_argument('--environment',type=Path)
    a=ap.parse_args();report(a.raw,a.out,a.figures,a.seeds,a.environment)
