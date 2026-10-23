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
    # Pure standard-library SVG: GitHub Ubuntu runners do not ship matplotlib.
    # The exact same code renders checked-in charts and independent CI charts.
    from xml.sax.saxutils import escape
    figdir=Path(a.figure_dir);figdir.mkdir(parents=True,exist_ok=True)
    palette=('#235789','#d76f39','#2d8f83','#7955a1')
    for metric,title,filename in [
        ('build_ns_per_key','Build latency (ns/key)','v0.11.0-appendix-b-build.svg'),
        ('hit_ns_per_op','Positive lookup latency (ns/op)','v0.11.0-appendix-b-hit.svg')]:
        width,height=960,540
        xmin,xmax=95,880; ymin,ymax=100,435
        groups=[]
        for capacity in ('4096','16384'):
            for method,short in [('advanced_s52_stored_choice','Stored choice'),
                                 ('advanced_appendix_b_choice_prime',"Appendix B choice'")]:
                items=sorted((r for r in out if r['capacity']==capacity and r['algorithm']==method),
                             key=lambda r:int(r['load_pct']))
                groups.append((f'{short} / {capacity} slots',items))
        ymaxval=max(float(r[f'mean_{metric}']) for _,items in groups for r in items)*1.13
        x=lambda v:xmin+(float(v)-70)/25*(xmax-xmin)
        y=lambda v:ymax-float(v)/ymaxval*(ymax-ymin)
        els=[f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" role="img" aria-label="{escape(title)}">',
             '<rect width="100%" height="100%" fill="#ffffff"/>',
             f'<text x="{width/2}" y="36" text-anchor="middle" font-family="sans-serif" font-size="19" fill="#222">SODA 2025 cloud experiment — {escape(title)}</text>',
             f'<path d="M {xmin} {ymin} V {ymax} H {xmax}" stroke="#333" stroke-width="1.4" fill="none"/>']
        for step in range(5):
            v=ymaxval*step/4; yy=y(v)
            els.extend([f'<line x1="{xmin}" y1="{yy:.1f}" x2="{xmax}" y2="{yy:.1f}" stroke="#eeeeee"/>',
                       f'<text x="{xmin-9}" y="{yy+5:.1f}" text-anchor="end" font-family="sans-serif" font-size="12" fill="#444">{v:.0f}</text>'])
        for load in (70,85,95):
            xx=x(load)
            els.extend([f'<line x1="{xx:.1f}" y1="{ymax}" x2="{xx:.1f}" y2="{ymax+6}" stroke="#444"/>',
                       f'<text x="{xx:.1f}" y="{ymax+23}" font-size="13" font-family="sans-serif" text-anchor="middle">{load}%</text>'])
        for idx,(label,items) in enumerate(groups):
            color=palette[idx]
            points=' '.join(f"{x(r['load_pct']):.1f},{y(r[f'mean_{metric}']):.1f}" for r in items)
            els.append(f'<polyline points="{points}" fill="none" stroke="{color}" stroke-width="2.6"/>')
            for r in items:
                els.append(f'<circle cx="{x(r["load_pct"]):.1f}" cy="{y(r[f"mean_{metric}"]):.1f}" r="4.5" fill="{color}"/>')
            xx=120+(idx%2)*400; yy=478+(idx//2)*23
            els.extend([f'<line x1="{xx}" y1="{yy}" x2="{xx+25}" y2="{yy}" stroke="{color}" stroke-width="3"/>',
                        f'<text x="{xx+32}" y="{yy+5}" font-family="sans-serif" font-size="13" fill="#333">{escape(label)}</text>'])
        els.append('</svg>')
        (figdir/filename).write_text('\n'.join(els)+'\n',encoding='utf-8')
    print(f'validated {len(rows)}/{len(expected)} real rows; plots generated')
if __name__=='__main__':main()
