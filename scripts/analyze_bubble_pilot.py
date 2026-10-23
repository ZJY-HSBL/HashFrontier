#!/usr/bin/env python3
"""Report from raw actual Section 4/5.2 mechanical pilot benchmark."""
import csv, statistics, math
from collections import defaultdict
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
plt.rcParams["svg.fonttype"] = "none"
rdir=Path(__file__).resolve().parent.parent/'results'
fdir=rdir.parent/'figures'
fdir.mkdir(exist_ok=True)
raw=list(csv.DictReader((rdir/'v0.9.0-bubble-pilot.csv').open()))
assert len(raw)==180
bins=defaultdict(list)
for r in raw:
    bins[(r['method'],int(r['load']))].append(r)
assert all(len(v)==30 and len({int(x['seed']) for x in v})==30 for v in bins.values())
metrics=['build_ns_per_key','hit_ns_per_op','miss_ns_per_op']
with (rdir/'v0.9.0-bubble-summary.csv').open('w',newline='') as f:
    fields=['method','load','seeds','completed','failed','mean_build_ns_per_key','mean_hit_ns_per_op','mean_miss_ns_per_op','sample_stddev_build_ns_per_key','sample_stddev_hit_ns_per_op']
    writer=csv.DictWriter(f,fieldnames=fields);writer.writeheader()
    for (a,load),v in sorted(bins.items()):
        means={f'mean_{k}':f'{statistics.mean(float(x[k]) for x in v):.5f}' for k in metrics}
        writer.writerow({'method':a,'load':load,'seeds':len(v),'completed':sum(x['status']=='complete' for x in v),'failed':sum(x['status']!='complete' for x in v),**means,
            'sample_stddev_build_ns_per_key':f'{statistics.stdev(float(x["build_ns_per_key"]) for x in v):.5f}',
            'sample_stddev_hit_ns_per_op':f'{statistics.stdev(float(x["hit_ns_per_op"]) for x in v):.5f}'})
fig,axs=plt.subplots(1,2,figsize=(10,4),dpi=150)
labels={'bubble_basic_s4':'Basic §4','bubble_advanced_s52':'Advanced §5.2'}
for j,(metric,ax) in enumerate(zip(['build_ns_per_key','hit_ns_per_op'],axs)):
    for name in labels:
        vals=[statistics.mean(float(x[metric]) for x in bins[(name,load)]) for load in (70,85,95)]
        ax.plot((70,85,95),vals,marker='o',label=labels[name],linewidth=2)
    ax.set_xticks((70,85,95));ax.set_xlabel('Occupancy (%)')
    ax.set_ylabel('Nanoseconds per '+('inserted key' if j==0 else 'successful query'))
    ax.grid(alpha=.25);ax.legend()
    ax.set_title(('Build' if j==0 else 'Positive lookup')+' | 8192 slots, 30 paired seeds')
fig.suptitle('SODA 2025 bubble-up: Section 4 vs Section 5.2 (finite engineering pilot)')
fig.tight_layout();fig.savefig(fdir/'v0.9.0-bubble-compare.svg',format='svg')
plt.close(fig)
print('180 observed rows validated; 6 group summaries; genuine SVG generated')
