#!/usr/bin/env python3
"""Audit exact DMPP06 v0.23.0 real observations; render standalone SVG."""
import csv, json, hashlib, statistics, argparse
from pathlib import Path
from collections import defaultdict
COLUMNS="method,k,bucket_bits,keys,seed,shape,collision_keys,max_bucket,metadata_bytes,ns_per_hash,checksum,valid".split(",")
METHODS=[("v022_explicit",0),("v023_poly",2),("v023_poly",8),("v023_poly",16)]
def validate(raw,out,seeds=5):
  with raw.open(newline="") as f:
    r=csv.DictReader(f)
    if r.fieldnames!=COLUMNS:raise ValueError("unexpected header")
    data=list(r)
  expected={(m,k,b,n,s,shape) for b in (8,10,12) for n in (1024,4096)
            for s in range(1,seeds+1) for shape in ("contiguous","uniform") for m,k in METHODS}
  seen=set();groups=defaultdict(list)
  for r in data:
    try:
      id=(r["method"],int(r["k"]),int(r["bucket_bits"]),int(r["keys"]),int(r["seed"]),r["shape"])
      hits=int(r["collision_keys"]);high=int(r["max_bucket"]);memory=int(r["metadata_bytes"])
      ns=float(r["ns_per_hash"]);checksum=int(r["checksum"]);valid=int(r["valid"])
    except (ValueError,KeyError) as e:raise ValueError("invalid row") from e
    if id not in expected or id in seen:raise ValueError("unexpected or duplicate case "+str(id))
    seen.add(id)
    m,k,b,n,seed,shape=id
    if not (0<=hits<=n and 1<=high<=n and (hits==0)==(high==1)):raise ValueError("occupancy")
    if not (0<ns<1e7 and checksum>=0 and valid==1):raise ValueError("timing or correctness")
    if memory!=(1024 if k==0 else 768+k):raise ValueError("memory accounting")
    groups[(m,k,b,n,shape)].append((hits,high,ns,memory))
  if seen!=expected:raise ValueError("missing "+str(len(expected-seen))+" cases")
  out.mkdir(parents=True,exist_ok=True)
  summary=[]
  for (m,k,b,n,shape),vals in sorted(groups.items()):
    if len(vals)!=seeds:raise ValueError("incomplete paired seeds")
    summary.append(dict(method=m,k=k,bucket_bits=b,keys=n,shape=shape,seeds=seeds,
       collision_keys_median=round(statistics.median(x[0] for x in vals),2),
       max_bucket_median=round(statistics.median(x[1] for x in vals),2),
       ns_per_hash_median=round(statistics.median(x[2] for x in vals),3),
       metadata_bytes=vals[0][3]))
  with (out/"summary.csv").open("w",newline="") as f:
    w=csv.DictWriter(f,fieldnames=list(summary[0]));w.writeheader();w.writerows(summary)
  audit={"expected":len(expected),"observed":len(data),"summary_groups":len(summary),
         "seeds":seeds,"source_csv_sha256":hashlib.sha256(raw.read_bytes()).hexdigest(),
         "status":"complete","formal_theorem_verified":False}
  (out/"audit.json").write_text(json.dumps(audit,indent=2)+"\n")
  for metric,file,title in [("collision_keys_median","collisions.svg","Collision-involved keys"),
                             ("ns_per_hash_median","hash-cost.svg","Measured ns/hash")]:
    group={(x["method"],int(x["k"])):x for x in summary
           if x["bucket_bits"]==10 and x["keys"]==4096 and x["shape"]=="uniform"}
    values=[float(group[id][metric]) for id in METHODS]
    ymax=max(values)*1.12 or 1
    svg=['<svg xmlns="http://www.w3.org/2000/svg" width="920" height="345" viewBox="0 0 920 345">',
         '<rect width="920" height="345" fill="white"/>',
         '<text x="25" y="34" font-family="Arial" font-size="20">'+title+' — 4096 keys, 1024 buckets, uniform</text>',
         '<line x1="60" y1="275" x2="900" y2="275" stroke="#333"/>']
    for i,(name,value) in enumerate(zip(("v0.22","k=2","k=8","k=16"),values)):
      x=88+205*i;h=185*value/ymax;y=275-h
      svg += [f'<rect x="{x}" y="{y:.2f}" width="130" height="{h:.2f}" fill="#326b94" rx="4"/>',
              f'<text x="{x+65}" y="{y-8:.2f}" text-anchor="middle" font-size="13" font-family="Arial">{value:.2f}</text>',
              f'<text x="{x+65}" y="300" text-anchor="middle" font-size="15" font-family="Arial">{name}</text>']
    svg+=['<text x="25" y="330" font-family="Arial" font-size="12" fill="#666">Actual five-seed medians; finite model, not the Siegel high-probability theorem.</text></svg>']
    (out/file).write_text("\n".join(svg)+"\n")
  return audit
if __name__=="__main__":
  a=argparse.ArgumentParser();a.add_argument("raw",type=Path);a.add_argument("out",type=Path);a.add_argument("--seeds",type=int,default=5)
  x=a.parse_args();print(json.dumps(validate(x.raw,x.out,x.seeds)))
