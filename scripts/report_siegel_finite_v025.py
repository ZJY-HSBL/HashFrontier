#!/usr/bin/env python3
"""Strict reporting of actual Siegel Lemma 2.6 finite graph trials.
No asymptotic independence or Siegel explicit-construction claims.
"""
import argparse,csv,json,statistics,collections
from pathlib import Path
FIELDS=['component','domain','right_cells','degree','k','seed','search_attempts','certified_subsets','evaluations','hash_ns_per_eval','graph_min_bytes','random_table_bytes','checksum','valid']
def audit(path):
    with open(path,newline='',encoding='utf-8') as f:
        r=csv.DictReader(f)
        if r.fieldnames!=FIELDS:raise ValueError('unexpected CSV schema')
        rows=list(r)
    if len(rows)!=40:raise ValueError(f'expected 40 rows, got {len(rows)}')
    seen=set();grouped=collections.defaultdict(list)
    for row in rows:
        if row['component']!='siegel_lemma2.6_finite':raise ValueError('wrong mechanism')
        n=int(row['domain']);v=int(row['right_cells']);d=int(row['degree']);k=int(row['k']);s=int(row['seed'])
        if n not in (16,24,32,64) or v!=2*n or d not in (3,4) or k!=4 or s not in (1,2,3,4,5):raise ValueError('unexpected parameters')
        key=(n,d,s)
        if key in seen:raise ValueError('duplicate trial')
        seen.add(key)
        if int(row['valid'])!=1 or int(row['evaluations'])!=120000 or int(row['search_attempts'])<1:raise ValueError('failed/missing run')
        from math import comb,isfinite
        if int(row['certified_subsets'])!=sum(comb(n,i) for i in range(1,k+1)):
            raise ValueError('wrong exhaustive certification count')
        if int(row['graph_min_bytes'])!=n*d*2 or int(row['random_table_bytes'])!=v*8:raise ValueError('metadata accounting incorrect')
        ns=float(row['hash_ns_per_eval'])
        if not isfinite(ns) or ns<=0 or ns>1e7:raise ValueError('invalid elapsed time')
        grouped[(n,d)].append(row)
    if len(grouped)!=8 or any(len(v)!=5 for v in grouped.values()):raise ValueError('incomplete groups')
    return rows,grouped

def svg(out,name,series,fmt):
    w=790;h=345;mx=95;bottom=275;top=47;right=755
    max_y=max(v for _,v in series)*1.2
    points=[]
    for i,(lbl,val) in enumerate(series):
        cx=mx+(right-mx)*(i+.5)/len(series)
        cy=bottom-(bottom-top)*val/max_y
        points.append((lbl,val,cx,cy))
    a=[f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">',
       '<rect width="790" height="345" fill="white"/>',
       f'<text x="42" y="26" font-family="Arial" font-size="17" font-weight="bold">{name}</text>',
       f'<path d="M {mx} {top} V {bottom} H {right}" fill="none" stroke="#667085"/>']
    for lab,val,cx,cy in points:
        a.append(f'<rect x="{cx-22:.1f}" y="{cy:.1f}" width="44" height="{bottom-cy:.1f}" fill="#376fbd"/>')
        a.append(f'<text x="{cx:.1f}" y="{cy-8:.1f}" text-anchor="middle" font-family="Arial" font-size="12">{fmt(val)}</text>')
        a.append(f'<text x="{cx:.1f}" y="{bottom+19}" text-anchor="middle" font-family="Arial" font-size="12">{lab}</text>')
    a.append('<text x="395" y="329" font-size="11" text-anchor="middle" font-family="Arial" fill="#475467">40 measured cloud trials; five seeds per configuration; finite explicit-adjacency graph only</text></svg>')
    Path(out).write_text('\n'.join(a)+'\n',encoding='utf-8')

def main():
    ap=argparse.ArgumentParser();ap.add_argument('raw');ap.add_argument('out');args=ap.parse_args()
    rows,groups=audit(args.raw);out=Path(args.out);out.mkdir(parents=True,exist_ok=True)
    summary=[]
    for (n,d),rr in sorted(groups.items()):
        summary.append({'domain':n,'right_cells':2*n,'degree':d,'k':4,'seeds':len(rr),
            'median_hash_ns_per_eval':round(statistics.median(float(x['hash_ns_per_eval']) for x in rr),5),
            'median_graph_search_attempts':statistics.median(int(x['search_attempts']) for x in rr),
            'graph_min_bytes':int(rr[0]['graph_min_bytes']),'random_table_bytes':int(rr[0]['random_table_bytes'])})
    with (out/'summary.csv').open('w',newline='',encoding='utf-8') as f:
        writer=csv.DictWriter(f,fieldnames=list(summary[0]));writer.writeheader();writer.writerows(summary)
    (out/'audit.json').write_text(json.dumps({'valid':True,'rows':len(rows),'configurations':8,'seeds_per_configuration':5,
       'certified_k':4,'only_finite_graph_instance':True,'full_Siegel_graph_construction':False,
       'graph_adjacency_explicit':True,'theorem_asymptotics_claimed':False,
       'certified_nonempty_subsets_each_trial':{str(n):sum(__import__('math').comb(n,i) for i in range(1,5)) for n in (16,24,32,64)}},indent=2)+'\n')
    svg(out/'hash.svg','Siegel finite graph XOR evaluation (ns/op)',[(f'{r["domain"]}d{r["degree"]}',r['median_hash_ns_per_eval']) for r in summary],lambda x:f'{x:.2f}')
    svg(out/'memory.svg','Finite explicit graph metadata minimum (bytes)',[(f'{r["domain"]}d{r["degree"]}',r['graph_min_bytes']+r['random_table_bytes']) for r in summary],lambda x:f'{x:.0f}')
    print(f'PASS 40/40 certified finite graph trials, 8 five-seed configs; output={out}')
if __name__=='__main__':main()
