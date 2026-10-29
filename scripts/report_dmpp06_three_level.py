#!/usr/bin/env python3
"""Strictly verify and visualize real finite DMPP06 three-tier measurements."""
from pathlib import Path
from collections import defaultdict
import argparse,csv,hashlib,json,statistics
COLUMNS=("bucket_bits_first,bucket_bits_second,first_codes,first_tiles,third_capacity,seed,attempted,"
"inserted_first,inserted_second,inserted_third,overflow,hit_queries,miss_queries,updates,erases,"
"build_ns_per_attempt,hit_ns_per_op,miss_ns_per_op,update_ns_per_op,erase_ns_per_op,valid").split(",")
TIMES=("build_ns_per_attempt","hit_ns_per_op","miss_ns_per_op","update_ns_per_op","erase_ns_per_op")
def audit(path):
 with path.open(newline="") as f:
  reader=csv.DictReader(f)
  if reader.fieldnames!=COLUMNS:raise ValueError("unexpected column header")
  rows=list(reader)
 if len(rows)!=96:raise ValueError(f"expected 96 observations, got {len(rows)}")
 seen=set();groups=defaultdict(list)
 for old in rows:
  if any(old.get(k) in (None,"") for k in COLUMNS):raise ValueError("missing cell")
  r={k:(float(v) if k in TIMES else int(v)) for k,v in old.items()}
  key=tuple(r[k] for k in ("bucket_bits_first","bucket_bits_second","first_codes","first_tiles","third_capacity","seed"))
  if key in seen:raise ValueError("duplicate run")
  seen.add(key)
  b1,b2,fc,tiles,cap,seed=key
  if b1 not in (4,6) or b2 not in (4,6) or fc not in (2,4) or tiles not in (2,4) or cap not in (64,512) or seed not in (1,2,3) or r["valid"]!=1:raise ValueError("illegal matrix point")
  accepted=sum(r[k] for k in ("inserted_first","inserted_second","inserted_third"))
  if accepted+r["overflow"]!=1600 or r["attempted"]!=1600 or r["hit_queries"]!=accepted*4 or r["miss_queries"]!=4096 or r["updates"]!=accepted//4 or r["erases"]!=accepted:raise ValueError("inconsistent operations")
  if any(not(0<=r[k]<1e9) for k in TIMES):raise ValueError("invalid timing")
  groups[key[:-1]].append(r)
 expected={(b1,b2,fc,tiles,cap,seed) for b1 in (4,6) for b2 in (4,6) for fc in (2,4) for tiles in (2,4) for cap in (64,512) for seed in (1,2,3)}
 if seen!=expected:raise ValueError("missing matrix cells")
 summary=[]
 for key,items in sorted(groups.items()):
  if len(items)!=3:raise ValueError("not three seeds")
  x=dict(zip(("bucket_bits_first","bucket_bits_second","first_codes","first_tiles","third_capacity"),key))
  x["seeds"]=3
  for col in ("inserted_first","inserted_second","inserted_third","overflow"):
   x[col+"_median"]=statistics.median(v[col] for v in items)
  for col in TIMES:
   x[col+"_median"]=round(statistics.median(v[col] for v in items),3)
  summary.append(x)
 return rows,summary
def svg(summary,column,path,title):
 selected=sorted((r for r in summary if r["bucket_bits_first"]==6 and r["bucket_bits_second"]==6 and r["first_codes"]==4 and r["first_tiles"]==4),key=lambda r:r["third_capacity"])
 if len(selected)!=2:raise ValueError("chart selection absent")
 bars=[];bottom=264;peak=max(float(x[column]) for x in selected)*1.2 or 1
 for i,r in enumerate(selected):
  x=170+i*270;y=float(r[column]);h=y/peak*180
  bars.append(f'<rect x="{x}" y="{bottom-h:.1f}" width="112" height="{h:.1f}" fill="{["#2176ae","#16a085"][i]}" rx="5"/><text x="{x+56}" y="{bottom-h-11:.1f}" text-anchor="middle" font-size="15">{y:.2f}</text><text x="{x+56}" y="{bottom+25}" text-anchor="middle" font-size="14">third cap {r["third_capacity"]}</text>')
 path.write_text(f'<svg xmlns="http://www.w3.org/2000/svg" width="760" height="340" viewBox="0 0 760 340"><rect width="760" height="340" fill="white"/><text x="380" y="35" font-size="20" text-anchor="middle">{title}</text><line x1="80" y1="264" x2="720" y2="264" stroke="#555"/>{''.join(bars)}<text x="380" y="322" text-anchor="middle" font-size="11">Three paired seeds; finite 16-bit cloud pilot, NOT full DMPP06</text></svg>')
def main(src,out):
 raw,summary=audit(src);out.mkdir(parents=True,exist_ok=True)
 with (out/"summary.csv").open("w",newline="") as f:
  w=csv.DictWriter(f,fieldnames=list(summary[0]));w.writeheader();w.writerows(summary)
 svg(summary,"inserted_third_median",out/"third.svg","Accepted into DMPP06 third tier")
 svg(summary,"overflow_median",out/"overflow.svg","All-tier overflow")
 audit_data=dict(source_rows=len(raw),groups=len(summary),sha256_raw=hashlib.sha256(src.read_bytes()).hexdigest(),
  scope="finite routing; std::unordered_map third-tier stand-in; not complete DMPP06")
 (out/"audit.json").write_text(json.dumps(audit_data,indent=2)+"\n")
 print(f"Valid finite three-tier observations: {len(raw)}, complete parameter groups: {len(summary)}")
if __name__=="__main__":
 p=argparse.ArgumentParser();p.add_argument("source",type=Path);p.add_argument("out",type=Path)
 a=p.parse_args();main(a.source,a.out)
