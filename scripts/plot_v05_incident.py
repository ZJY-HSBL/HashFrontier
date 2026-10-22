#!/usr/bin/env python3
"""Visual audit of v0.4.0 failed author-native CI; no fabricated observations."""
import csv
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
ROOT=Path(__file__).resolve().parents[1]
with (ROOT/'results/v0.5.0-v04-incident.csv').open(newline='') as f:
    rows=list(csv.DictReader(f))
assert len(rows)==16
fig,ax=plt.subplots(figsize=(10.3,5.4))
labels=[];counts=[];states=[]
for row in rows:
    labels.append(f"{row['load']}% {row['repeat']} {row['variant']}")
    counts.append(int(row['completed_cycles']))
    states.append(row['status'])
ys=list(range(len(rows)))
ax.barh(ys,counts,edgecolor='0.4',linewidth=0.7)
for i,(count,state) in enumerate(zip(counts,states)):
    if state=='not_executed':
        ax.text(4,i,'NOT EXECUTED',ha='left',va='center',fontsize=8)
    elif state!='complete':
        ax.text(count+3,i,f'{count}/320 — ABORTED',ha='left',va='center',fontsize=8)
    else:
        ax.text(count+3,i,'320/320',ha='left',va='center',fontsize=8)
ax.set_yticks(ys,labels,fontsize=8);ax.invert_yaxis();ax.set_xlim(0,400)
ax.set_xlabel('Actually completed full DELETE + INSERT + LOOKUP cycles')
ax.set_title('v0.4.0 author-native CI failure: 85% completed; 95% aborted / not run')
ax.grid(axis='x',alpha=.25);fig.tight_layout()
path=ROOT/'figures/v0.5.0-v04-incident.svg';fig.savefig(path,format='svg');plt.close(fig)
print(path)
