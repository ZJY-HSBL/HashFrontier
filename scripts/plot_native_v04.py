#!/usr/bin/env python3
"""Plot source-backed, unpaired author-native ZombieHT evidence without ranking it."""
from pathlib import Path
import csv
import statistics as stats
from collections import defaultdict
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

ROOT=Path(__file__).resolve().parents[1]
DATA=ROOT/'results/v0.4.0-author-v03-audit'
OUT=ROOT/'figures'
OUT.mkdir(exist_ok=True)
with (DATA/'native-cycles.csv').open(newline='') as f:
    records=list(csv.DictReader(f))
with (DATA/'audit.csv').open(newline='') as f:
    audit={r['variant']:r for r in csv.DictReader(f)}
assert len(records)==3*(320*3+194)
methods=['RHM','TRHM','GZHM','GZHM_ADAPTIVE']
phases=['DELETE','INSERT','LOOKUP']
summary=[]
for variant in methods:
    for phase in phases:
        vals=[float(x['clock_ticks_per_op']) for x in records if x['variant']==variant and x['phase']==phase]
        ordered=sorted(vals)
        summary.append({'variant':variant,'phase':phase,'complete':audit[variant]['complete'],
                        'cycles':len(vals),'median_clock_ticks_per_op':round(stats.median(vals),3),
                        'p99_clock_ticks_per_op':round(ordered[int(.99*(len(ordered)-1))],3),
                        'mean_clock_ticks_per_op':round(stats.mean(vals),3)})
with (DATA/'native-summary.csv').open('w',newline='') as f:
    writer=csv.DictWriter(f,fieldnames=list(summary[0]));writer.writeheader();writer.writerows(summary)
fig,ax=plt.subplots(figsize=(10,5.7))
for variant in methods:
    vals=[];xs=[]
    n=int(audit[variant]['completed_cycles'])
    for start in range(0,n,16):
        v=[float(r['clock_ticks_per_op']) for r in records if r['variant']==variant and r['phase']=='INSERT' and start<=int(r['cycle'])<start+16]
        if v:xs.append(start+len(v));vals.append(stats.median(v))
    ax.plot(xs,vals,linewidth=1.7,label=f'{variant} ({n}/320 complete)')
ax.set(xlabel='Completed native churn cycle',ylabel='Median insertion clock ticks / operation (16-cycle bins)',
       title='ZombieHT author-native scaled q=13, 95% load | unpaired OpenSSL traces')
ax.grid(alpha=.24);ax.legend();fig.tight_layout();fig.savefig(OUT/'v0.4.0-native-insert-trends.svg');plt.close(fig)
fig,axes=plt.subplots(1,3,figsize=(12,4.1),sharey=False)
for ax,phase in zip(axes,phases):
    names=[m for m in methods]
    vals=[next(r['median_clock_ticks_per_op'] for r in summary if r['variant']==m and r['phase']==phase) for m in names]
    ax.bar(range(len(names)),vals);ax.set_xticks(range(len(names)),names,rotation=35,ha='right');ax.set_title(phase)
    ax.set_ylabel('Median native clock ticks/op')
    ax.grid(axis='y',alpha=.2)
fig.suptitle('Descriptive only: independent unpaired runs; TRHM incomplete at cycle 194')
fig.tight_layout();fig.savefig(OUT/'v0.4.0-native-phase-median.svg');plt.close(fig)
print('author-native v0.3.0 archive: four variants; three complete, TRHM only 194/320; visualizations and summary saved')
