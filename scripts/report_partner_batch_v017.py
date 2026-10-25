#!/usr/bin/env python3
"""Real Algorithm-5 finite bench audit. Old/new do NOT complete same work."""
import csv,json,statistics,sys
from pathlib import Path
raw=Path(sys.argv[1]);out=Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
with raw.open(newline="") as f: rows=list(csv.DictReader(f))
if len(rows)!=54:raise ValueError("expected 54 actual runs")
seen=set(); grouped={}
for r in rows:
    k=(r["algorithm"],int(r["bins"]),int(r["B"]),int(r["seed"]))
    if k in seen or k[0] not in ("disjoint_old","prestate_reuse") or k[1] not in (8,16,32) or k[2] not in (16,32,64) or k[3] not in (1,2,3):
        raise ValueError("unexpected/duplicate key")
    seen.add(k)
    n=int(r["attempts"]); ok=int(r["committed"]); bad=int(r["rejected"])
    attempted=int(r["mutation_attempts"]); completed=int(r["mutation_committed"])
    if not (n==1500 and ok+bad==n and attempted+int(r["noops"])==n and int(r["valid"])==1 and int(r["encoded_words"])==k[2]*2 and 0<=completed<=attempted and completed<=ok):
        raise ValueError("corrupt or incomplete experiment")
    if k[0]=="prestate_reuse" and (ok!=n or completed!=attempted or int(r["cross_bin_reuse"])==0):
        raise ValueError("reusing method failed full-bin swap")
    if k[0]=="disjoint_old" and (completed!=0 or bad==0):
        raise ValueError("old method unexpectedly completed a mutation")
    grouped.setdefault((k[0],k[2]),[]).append(r)
summary=[]
for (method,B), rr in sorted(grouped.items()):
    attempts=sum(int(r["mutation_attempts"]) for r in rr)
    completed=sum(int(r["mutation_committed"]) for r in rr)
    summary.append(dict(method=method,B=B,seeded_runs=len(rr),mutation_attempts=attempts,mutation_committed=completed,
                        mutation_completion_pct=round(100*completed/attempts,2),
                        median_ns_per_attempt=round(statistics.median(float(r["update_ns_per_attempt"]) for r in rr),3)))
with (out/"summary.csv").open("w",newline="") as f:
    w=csv.DictWriter(f,fieldnames=list(summary[0]));w.writeheader();w.writerows(summary)
(out/"audit.json").write_text(json.dumps({"rows":len(rows),"valid":True,"track":"finite Algorithm 5 component, external retrieval map remains",
 "warning":"legacy rejects actual mutations, so absolute ns/attempt MUST NOT be used as an equivalent-work speed comparison"},indent=2))
def chart(file,key,title):
    svg=['<svg xmlns="http://www.w3.org/2000/svg" width="780" height="390" viewBox="0 0 780 390">',
         '<rect width="780" height="390" fill="white"/>',
         '<text x="390" y="32" text-anchor="middle" font-family="Arial" font-size="18">'+title+'</text>',
         '<path d="M80 55 V330 H730" fill="none" stroke="black"/>']
    maximum=max(float(r[key]) for r in summary)*1.2 or 1
    for ix,(method,color) in enumerate((("disjoint_old","#BB4A4A"),("prestate_reuse","#207A70"))):
        pts=[]
        for i,B in enumerate((16,32,64)):
            r=next(r for r in summary if r["method"]==method and r["B"]==B)
            x=190+i*200;y=330-float(r[key])/maximum*260;pts.append((x,y))
            svg.append(f'<circle cx="{x}" cy="{y:.1f}" r="5" fill="{color}"/>')
            svg.append(f'<text x="{x}" y="353" text-anchor="middle" font-family="Arial" font-size="14">{B}</text>')
        svg.append('<polyline points="'+' '.join(f'{x},{y:.1f}' for x,y in pts)+f'" fill="none" stroke="{color}" stroke-width="3"/>')
        svg.append(f'<text x="{85+ix*300}" y="58" fill="{color}" font-family="Arial" font-size="13">{method}</text>')
    svg.append('</svg>')
    (out/file).write_text('\n'.join(svg))
chart("success.svg","mutation_completion_pct","Full-bin mutating batches committed (%)")
chart("attempt.svg","median_ns_per_attempt","Time per attempt — NOT equal completed work (ns)")
print("verified",len(rows),"cloud measurements")
