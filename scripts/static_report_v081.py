#!/usr/bin/env python3
"""Strict same-trace static construction/query comparison with failure accounting."""
import argparse,csv,math,statistics
from collections import defaultdict
from pathlib import Path
ALGORITHMS=('std_unordered_set','tpht_flatten','tpht_chained','zombie_gzhm','zombie_gzhm_adaptive','bubble_up_basic')
METRICS=('build_ns_per_key','hit_ns_per_op','miss_ns_per_op')

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--observations',type=Path,required=True);ap.add_argument('--completeness',type=Path,required=True);ap.add_argument('--out',type=Path,required=True);a=ap.parse_args()
 with a.observations.open(newline='') as f:obs=list(csv.DictReader(f))
 with a.completeness.open(newline='') as f:audit=list(csv.DictReader(f))
 if len(audit)!=180:raise ValueError(f'expected exactly 180 static runs, found {len(audit)}')
 seen=set();records={}
 for r in audit:
  key=(r['algorithm'],r['scenario'],int(r['seed']))
  if key in seen:raise ValueError('duplicate audit record')
  seen.add(key)
  if r['algorithm'] not in ALGORITHMS or r['scenario'] not in ('static_load_70','static_load_85','static_load_95') or int(r['seed']) not in range(101,111):raise ValueError('unexpected run')
 for r in obs:
  key=(r['algorithm'],r['scenario'],int(r['seed']))
  if key in records:raise ValueError('duplicate result')
  if key not in seen or r['status']!='complete' or r['correct']!='true':raise ValueError('invalid result')
  for m in METRICS:
   x=float(r[m]);
   if not (x>0 and math.isfinite(x)):raise ValueError('invalid metric')
  if len(r['trace_sha256'])!=64 or len(r['query_sha256'])!=64:raise ValueError('invalid SHA')
  records[key]=r
 for r in audit:
  key=(r['algorithm'],r['scenario'],int(r['seed']))
  if (r['status']=='complete')!=(key in records):raise ValueError('audit vs observation mismatch')
  if key in records and r['trace_sha256']!=records[key]['trace_sha256']:raise ValueError('trace SHA mismatch')
 for scene in ('static_load_70','static_load_85','static_load_95'):
  for seed in range(101,111):
   subset=[r for k,r in records.items() if k[1]==scene and k[2]==seed]
   if len({r['trace_sha256'] for r in subset})>1 or len({r['query_sha256'] for r in subset})>1:raise ValueError('UNPAIRED run')
 result=[]
 for alg in ALGORITHMS:
  for pct in (70,85,95):
   sub=[r for k,r in records.items() if k[0]==alg and k[1]==f'static_load_{pct}']
   row=dict(algorithm=alg,load_pct=pct,completed=len(sub),planned=10)
   for m in METRICS:row[m]=f'{statistics.mean(float(r[m]) for r in sub):.5f}' if sub else ''
   result.append(row)
 a.out.mkdir(parents=True,exist_ok=True)
 with (a.out/'static-summary.csv').open('w',newline='') as f:
  w=csv.DictWriter(f,fieldnames=list(result[0]));w.writeheader();w.writerows(result)
 # SVG: static successful lookup comparison. Only observed complete groups shown.
 w,h=1000,490;colors=('#566B9F','#348C81','#B078A0','#C78650','#9D9F52','#A95365');bars=[]
 for j,pct in enumerate((70,85,95)):
  grp=[r for r in result if r['load_pct']==pct and r['hit_ns_per_op']]
  vmax=max((float(r['hit_ns_per_op']) for r in grp),default=1)
  for i,r in enumerate(grp):
   x=30+320*j+i*48;y=385-300*float(r['hit_ns_per_op'])/vmax
   bars.append(f'<rect x="{x}" y="{y:.1f}" width="35" height="{385-y:.1f}" fill="{colors[ALGORITHMS.index(r["algorithm"])]}"/><text x="{x+17}" y="{y-6:.1f}" text-anchor="middle" font-size="11">{float(r["hit_ns_per_op"]):.0f}</text>')
  bars.append(f'<text x="{155+320*j}" y="414" text-anchor="middle" font-size="16">{pct}% load</text>')
 legends=''.join(f'<rect x="{20+i*165}" y="453" width="11" height="11" fill="{colors[i]}"/><text x="{35+i*165}" y="464" font-size="11">{alg.replace("_"," ")}</text>' for i,alg in enumerate(ALGORITHMS))
 svg=f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {w} {h}" width="{w}" height="{h}"><rect width="100%" height="100%" fill="white"/><text x="20" y="30" font-family="sans-serif" font-size="20">HashFrontier v0.8.1: construction-only successful lookup (ns/op)</text><text x="20" y="55" font-size="12">Identical initial keys and static query traces; 10 paired seeds; lower is better; exploratory.</text><line x1="20" y1="385" x2="985" y2="385" stroke="#888"/>{"".join(bars)}{legends}</svg>'
 (a.out/'static-comparison.svg').write_text(svg)
 print(f'STATIC {len(obs)}/{len(audit)} complete; {len(records)} validated observations')
if __name__=='__main__':main()
