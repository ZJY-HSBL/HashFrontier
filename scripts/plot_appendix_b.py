#!/usr/bin/env python3
"""Plot only actual finite experiments; verify complete paired matrix before plots."""
import argparse
import csv
from collections import defaultdict
from pathlib import Path
import statistics

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--input',default='results/v0.11.0-appendix-b-cloud-pilot.csv')
    p.add_argument('--summary',default='results/v0.11.0-appendix-b-summary.csv')
    p.add_argument('--figure-dir',default='figures')
    p.add_argument('--seeds',type=int,default=10)
    a=p.parse_args()
    rows=list(csv.DictReader(open(a.input,encoding='utf8')))
    expected={(variant,capacity,load,seed) for variant in
          ('advanced_s52_stored_choice','advanced_appendix_b_choice_prime')
         for capacity in ('4096','16384') for load in ('70','85','95')
         for seed in range(101,101+a.seeds)}
    actual={(r['algorithm'],r['capacity'],r['load_pct'],int(r['seed'])) for r in rows}
    if len(rows)!=len(expected) or actual!=expected or any(r['correct']!='1' for r in rows):
        raise SystemExit('invalid matrix or oracle failure')
    grouped=defaultdict(list)
    for row in rows:grouped[row['capacity'],row['load_pct'],row['algorithm']].append(row)
    fields=('build_ns_per_key','hit_ns_per_op','miss_ns_per_op','bytes_per_key','evictions','core_collision_redirects','choice_prime_evaluations','phases')
    out=[]
    for (capacity,load,method),group in sorted(grouped.items()):
        aggregate={'capacity':capacity,'load_pct':load,'algorithm':method,'n':len(group),
                   'completed':sum(x['complete']=='1' for x in group)}
        for k in fields:aggregate[f'mean_{k}']=statistics.mean(float(x[k]) for x in group)
        out.append(aggregate)
    path=Path(a.summary);path.parent.mkdir(parents=True,exist_ok=True)
    with path.open('w',newline='',encoding='utf8') as file:
        writer=csv.DictWriter(file,fieldnames=list(out[0]));writer.writeheader();writer.writerows(out)
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    figdir=Path(a.figure_dir);figdir.mkdir(parents=True,exist_ok=True)
    for metric,title,filename in [
            ('build_ns_per_key','Build latency (ns/key)','v0.11.0-appendix-b-build.svg'),
            ('hit_ns_per_op','Positive lookup latency (ns/op)','v0.11.0-appendix-b-hit.svg')]:
        fig,ax=plt.subplots(figsize=(8,4.7))
        for capacity in ['4096','16384']:
            for method,short in [('advanced_s52_stored_choice','Stored choice'),
                                 ('advanced_appendix_b_choice_prime',"Appendix B choice'")]:
                group=sorted((r for r in out if r['capacity']==capacity and r['algorithm']==method),key=lambda r:int(r['load_pct']))
                ax.plot([int(r['load_pct']) for r in group],
                        [r[f'mean_{metric}'] for r in group],marker='o',
                        label=f'{short} / {capacity} slots')
        ax.set(xlabel='Target load (%)',ylabel=title,title=f'SODA 2025 finite cloud-container pilot: {title}')
        ax.set_xticks([70,85,95]);ax.grid(alpha=.25);ax.legend(fontsize=8)
        fig.tight_layout();fig.savefig(figdir/filename);plt.close(fig)
    print(f'validated {len(rows)}/{len(expected)} real rows; plots generated')
if __name__=='__main__':main()
