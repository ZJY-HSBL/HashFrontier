#!/usr/bin/env python3
"""Strict finite cloud-pilot audit and actual-data SVG for STOC25 §3.1 primitive."""
import argparse,csv,html,json,math,statistics
from collections import defaultdict
from pathlib import Path

def analyze(raw:Path,out:Path,seeds:int):
    with raw.open(newline='') as f:rows=list(csv.DictReader(f))
    expected={(b,B,s) for b in (32,64,128) for B in (64,128,256) for s in range(1,seeds+1)}
    found=set(); bad=[]
    for r in rows:
        key=(int(r['bins']),int(r['bin_size']),int(r['seed']))
        if key in found:bad.append(f'duplicate {key}')
        found.add(key)
        if r['component']!='partner_ram_3.1' or int(r['slots'])!=key[0]*key[1] or int(r['valid'])!=1:
            bad.append(f'wrong component/correctness: {key}')
        if int(r['write_attempts'])!=int(r['successful_writes'])+int(r['failed_writes']):
            bad.append(f'bad completion: {key}')
        if int(r['write_attempts'])!=12000 or int(r['read_ops'])!=120000:bad.append(f'incorrect operations: {key}')
        for metric in ('write_ns_per_attempt','read_ns_per_op'):
            n=float(r[metric]);
            if not math.isfinite(n) or n<=0:bad.append(f'bad timing {key}/{metric}')
        if int(r['external_metadata_lower_bound_bytes'])<=0:
            bad.append('unreported external metadata')
    if found!=expected:bad.append(f'seeds/configuration mismatch: missing={sorted(expected-found)[:10]} extra={sorted(found-expected)[:10]}')
    if bad:raise ValueError('; '.join(bad))
    groups=defaultdict(list)
    for r in rows:groups[(int(r['bins']),int(r['bin_size']))].append(r)
    summary=[]
    for (bins,B),arr in sorted(groups.items()):
        summary.append(dict(bins=bins,bin_size=B,slots=bins*B,seeds=len(arr),
            write_ns_per_attempt_median=round(statistics.median(float(r['write_ns_per_attempt']) for r in arr),3),
            read_ns_per_op_median=round(statistics.median(float(r['read_ns_per_op']) for r in arr),3),
            failed_writes_sum=sum(int(r['failed_writes']) for r in arr),
            physical_bytes=int(arr[0]['physical_bytes']),
            external_metadata_lower_bound_bytes=int(arr[0]['external_metadata_lower_bound_bytes'])))
    out.mkdir(parents=True,exist_ok=True)
    with (out/'summary.csv').open('w',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=list(summary[0]));writer.writeheader();writer.writerows(summary)
    for field,title,filename in [('read_ns_per_op_median','ReadRAM (Algorithm 2)','read.svg'),('write_ns_per_attempt_median','WriteRAM (Algorithm 3)','write.svg')]:
        width=960;height=520;margin=74;maxval=max(float(r[field]) for r in summary)*1.20
        colors={32:'#2563eb',64:'#d97706',128:'#059669'}
        lines=[f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
               '<rect width="100%" height="100%" fill="#ffffff"/>',
               f'<text x="{margin}" y="45" font-size="24" fill="#0f172a" font-family="sans-serif">STOC 2025 §3.1 — {html.escape(title)}</text>',
               '<text x="74" y="66" font-size="13" fill="#475569" font-family="sans-serif">Measured cloud pilot — median across paired seeds; external retrieval oracle NOT eliminated</text>',
               f'<path d="M{margin} 90 V{height-margin} H{width-35}" fill="none" stroke="#475569" stroke-width="1.5"/>']
        data=sorted(summary,key=lambda r:(r['slots'],r['bins']))
        groups_x=sorted(set(x['slots'] for x in summary))
        for t in range(6):
            val=maxval*t/5;y=height-margin-(height-margin-100)*t/5
            lines.append(f'<path d="M{margin} {y:.1f} H{width-35}" stroke="#e2e8f0"/>')
            lines.append(f'<text x="{margin-9}" y="{y+5:.1f}" text-anchor="end" font-size="12" font-family="sans-serif" fill="#334155">{val:.0f}</text>')
        for i,size in enumerate(groups_x):
            x=margin+((width-margin-50)*(i+.5)/len(groups_x))
            lines.append(f'<text x="{x:.1f}" y="{height-margin+25}" text-anchor="middle" font-size="12" font-family="sans-serif" fill="#334155">{size:,}</text>')
            arr=[a for a in data if a['slots']==size];
            for j,a in enumerate(arr):
                v=float(a[field]);off=(j-(len(arr)-1)/2)*18; xx=x+off
                y=height-margin-(height-margin-100)*v/maxval
                lines.append(f'<line x1="{xx:.1f}" y1="{height-margin}" x2="{xx:.1f}" y2="{y:.1f}" stroke="{colors[a["bins"]]}" stroke-width="12"/>')
        lines += [f'<text x="{width/2}" y="{height-8}" font-size="15" text-anchor="middle" font-family="sans-serif">Physical slots (n)</text>',
                  f'<text x="19" y="{height/2}" text-anchor="middle" transform="rotate(-90 19 {height/2})" font-size="14" font-family="sans-serif">ns per operation</text>']
        for j,b in enumerate((32,64,128)):
            lines.append(f'<rect x="{width-300+j*95}" y="88" width="15" height="10" fill="{colors[b]}"/>')
            lines.append(f'<text x="{width-281+j*95}" y="98" font-size="12" font-family="sans-serif">bins={b}</text>')
        lines.append('</svg>')
        (out/filename).write_text('\n'.join(lines)+'\n',encoding='utf-8')
    info={'schema':'partner-hashing-section-3.1-finite-pilot-v1','paper':'STOC 2025 Optimal Non-Oblivious Open Addressing',
          'coverage':'Section 3.1 Algorithms 1/2/3 only: partner swaps & RAM encoding',
          'not_implemented':['retrieval data structure internally encoded','Section 4 RAM & backyard','Section 5 dynamic resizing','paper O(1) theorem'],
          'data_rows':len(rows),'configurations':len(summary),'seeds':seeds,'all_checks_passed':True,
          'failed_write_attempts':sum(int(r['failed_writes']) for r in rows),
          'warning':'external logical-slot oracle and masks are allocated in RAM outside the physical table; do not label this full-space partner hashing.'}
    (out/'audit.json').write_text(json.dumps(info,indent=2)+'\n')
    return info,summary

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--raw',type=Path,required=True);ap.add_argument('--out',type=Path,required=True)
    ap.add_argument('--seeds',type=int,default=5);args=ap.parse_args()
    i,s=analyze(args.raw,args.out,args.seeds)
    print(json.dumps(i,indent=2));print('mean configurations',len(s))
