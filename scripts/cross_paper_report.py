#!/usr/bin/env python3
"""Strict validation and transparent exploratory report for same-trace paper implementations."""
from __future__ import annotations
import argparse
import csv
import math
import statistics
from collections import defaultdict
from pathlib import Path

METRICS = ('build_ns_per_key','hit_ns_per_op','miss_ns_per_op','insert_ns_per_op','delete_ns_per_op','update_p99_ns')
ALGORITHMS = ('std_unordered_set','tpht_flatten','tpht_chained','zombie_gzhm','zombie_gzhm_adaptive')


def load_csv(path):
    with Path(path).open(newline='', encoding='utf-8') as f:
        return list(csv.DictReader(f))


def verify(rows, completeness):
    if not rows: raise ValueError('No completed measurements')
    runs = {}
    for row in rows:
        key = (row['algorithm'],row['scenario'],int(row['seed']))
        if key in runs: raise ValueError('duplicate observation: '+str(key))
        if row['status']!='complete' or row['correct']!='true':raise ValueError('non-complete row claimed measurement')
        for field in METRICS:
            value = float(row[field])
            if not math.isfinite(value) or value<=0:raise ValueError('invalid metric '+field)
        if len(row['trace_sha256'])!=64:raise ValueError('invalid sha256')
        runs[key]=row
    for pair in {(r['scenario'],r['seed']) for r in rows}:
        sha={r['trace_sha256'] for r in rows if (r['scenario'],r['seed'])==pair}
        if len(sha)!=1:raise ValueError('UNPAIRED trace '+str(pair))
    if completeness:
        seen=set()
        for r in completeness:
            key=(r['algorithm'],r['scenario'],int(r['seed']))
            if key in seen:raise ValueError('duplicate completeness')
            seen.add(key)
            present=key in runs
            if (r['status']=='complete') != present:raise ValueError('inconsistent completion '+str(key))
            if present and r['trace_sha256']!=runs[key]['trace_sha256']:raise ValueError('fingerprint mismatch')
        if len(seen)!=45:raise ValueError('missing planned runs: expected 45, got '+str(len(seen)))
    return runs


def summary(rows, completeness):
    runs=verify(rows,completeness)
    output=[]
    for alg in ALGORITHMS:
        for load in (70,85,95):
            sub=[r for r in rows if r['algorithm']==alg and int(r['load_pct'])==load]
            v={'algorithm':alg,'load_pct':load,'completed_seeds':len(sub),'planned_seeds':3}
            v.update({f'mean_{m}': f'{statistics.mean(float(r[m]) for r in sub):.4f}' if sub else '' for m in METRICS})
            # Native TPHT and Zombie memory reports differ in semantics: informational only.
            v['reported_memory_bytes_mean']=f'{statistics.mean(int(r["author_reported_memory_bytes"]) for r in sub):.0f}' if sub else ''
            output.append(v)
    return output


def write_svg(summary_rows,path):
    # Display successful/unsuccessful lookup data only, no fabricated bars for failures.
    width,height=970,540
    positions={70:170,85:470,95:770}
    colors=dict(zip(ALGORITHMS,('#4A90A4','#6B9A68','#9C7FBB','#CE8E62','#A6A05D')))
    rects=[]
    for load in (70,85,95):
        base=positions[load]
        sub=[r for r in summary_rows if r['load_pct']==load and r['mean_miss_ns_per_op']]
        maximum=max([float(r['mean_miss_ns_per_op']) for r in sub] or [1])
        for i,r in enumerate(sub):
            value=float(r['mean_miss_ns_per_op']);x=base-120+i*44;y=430-min(320,300*value/maximum)
            rects.append(f'<rect x="{x}" y="{y:.1f}" width="32" height="{430-y:.1f}" fill="{colors[r["algorithm"]]}"/><text x="{x+16}" y="{y-5:.1f}" text-anchor="middle" font-size="10">{value:.0f}</text>')
    legend=''.join(f'<rect x="{25+i*190}" y="492" width="13" height="13" fill="{colors[a]}"/><text x="{44+i*190}" y="503" font-size="12">{a.replace("_"," ")}</text>' for i,a in enumerate(ALGORITHMS))
    ticks=''.join(f'<text x="{positions[l]}" y="455" text-anchor="middle" font-size="14">{l}% occupancy</text>' for l in (70,85,95))
    svg=f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}"><rect width="100%" height="100%" fill="white"/><text x="28" y="35" font-size="21" font-family="sans-serif">HashFrontier v0.7.0: unsuccessful lookup ns/op</text><text x="28" y="57" font-size="13" fill="#555">Author-source fixed-trace pilot, 3 seeds; exploratory only. Lower is better.</text><line x1="30" y1="430" x2="930" y2="430" stroke="#888"/>{ticks}{"".join(rects)}{legend}</svg>'
    Path(path).write_text(svg,encoding='utf-8')


def main():
    p=argparse.ArgumentParser();p.add_argument('--preflight');p.add_argument('--observations');p.add_argument('--completeness');p.add_argument('--out');args=p.parse_args()
    if args.preflight:
        path=Path(args.preflight);rows=[]
        for line in path.read_text(encoding='utf-8').splitlines():
            fields=line.split(',')
            if len(fields)!=23 or fields[13]!='true' or fields[12]!='complete':
                raise ValueError('preflight failed row: '+line[:200])
            rows.append(fields)
        if len(rows)!=5 or len({r[11] for r in rows})!=1:
            raise ValueError('preflight incomplete/unpaired')
        print('PASS preflight: five runs, same trace, all correct');return
    if not(args.observations and args.completeness and args.out):p.error('supply both inputs and --out')
    a=load_csv(args.observations);b=load_csv(args.completeness)
    results=summary(a,b)
    out=Path(args.out);out.mkdir(exist_ok=True,parents=True)
    with (out/'summary.csv').open('w',newline='',encoding='utf-8') as f:
        wr=csv.DictWriter(f,fieldnames=list(results[0]));wr.writeheader();wr.writerows(results)
    write_svg(results,out/'comparison.svg')
    print('Completed:',len(a),'/',len(b),'; distinct paper implementations:',len({r['algorithm'] for r in a}))

if __name__=='__main__':main()
