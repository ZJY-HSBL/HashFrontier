#!/usr/bin/env python3
"""Analyze real source-native ZombieHT churn logs, preserving partial/failing runs.

Do not read zeros as success: all expected phases and the exit code must agree.
Clock units are the author's *clock ticks*, never relabeled as nanoseconds.
This analysis deliberately excludes incomplete runs from timing summaries.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import statistics
import sys

sys.path.insert(0,str(Path(__file__).resolve().parent))
from audit_zombie_native import audit_log

VARIANTS=('RHM','TRHM','GZHM','GZHM_ADAPTIVE')
PHASES=('DELETE','INSERT','LOOKUP')


def write_csv(path, rows, columns):
    path.parent.mkdir(parents=True,exist_ok=True)
    with path.open('w',newline='') as f:
        w=csv.DictWriter(f,fieldnames=columns,extrasaction='ignore')
        w.writeheader()
        w.writerows(rows)


def svg_chart(rows,output):
    """Native measured clock ticks, not synthetic numbers or ns conversions."""
    phases=['DELETE','INSERT','LOOKUP']
    variants=list(VARIANTS)
    loads=sorted({int(r['load_pct']) for r in rows})
    width,height=980,450
    left,top,right,bottom=78,52,30,100
    area=(width-left-right)/len(loads)
    vals=[float(r['median_last32_ticks_per_op']) for r in rows]
    ymax=max(vals+[1.0])*1.14
    colors={'RHM':'#26639e','TRHM':'#d38f2f','GZHM':'#26816b','GZHM_ADAPTIVE':'#8055ad'}
    escape=lambda x: str(x).replace('&','&amp;').replace('<','&lt;')
    s=[f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
       '<rect width="100%" height="100%" fill="#fff"/>',
       '<text x="40" y="30" font-family="sans-serif" font-size="19" fill="#172b40">Zombie Hashing — verified complete runs only</text>',
       '<text x="40" y="48" font-family="sans-serif" font-size="12" fill="#637282">Upstream instrumented benchmark; median last 32 cycles; CLOCK TICKS per op (not nanoseconds)</text>']
    for i in range(5):
        yy=top+(height-top-bottom)*(1-i/4)
        v=ymax*i/4
        s.append(f'<line x1="{left}" y1="{yy:.2f}" x2="{width-right}" y2="{yy:.2f}" stroke="#e5e8ea"/>')
        s.append(f'<text x="{left-8}" y="{yy+4:.2f}" font-size="11" text-anchor="end" font-family="sans-serif">{v:.0f}</text>')
    for li,load in enumerate(loads):
        cluster=[r for r in rows if int(r['load_pct'])==load and r['phase']=='INSERT']
        for vi,variant in enumerate(variants):
            row=next((r for r in cluster if r['variant']==variant),None)
            if row:
                v=float(row['median_last32_ticks_per_op'])
                bw=area*0.16
                x=left+li*area+area*.13+vi*(bw+area*.03)
                h=(height-top-bottom)*v/ymax
                y=height-bottom-h
                s.append(f'<rect x="{x:.2f}" y="{y:.2f}" width="{bw:.2f}" height="{h:.2f}" fill="{colors[variant]}"/>')
        s.append(f'<text x="{left+(li+.5)*area:.2f}" y="{height-bottom+23}" text-anchor="middle" font-family="sans-serif" font-size="13">{load}% load</text>')
    for vi,v in enumerate(variants):
        x=left+vi*215;y=height-50
        s.append(f'<rect x="{x}" y="{y}" width="14" height="14" fill="{colors[v]}"/>')
        s.append(f'<text x="{x+21}" y="{y+12}" font-size="12" font-family="sans-serif">{escape(v)}</text>')
    s.append('<text x="55" y="390" font-size="12" font-family="sans-serif" fill="#637282">Bars exclude any incomplete run; no survivor-only claim for the full cohort</text>')
    s.append('</svg>')
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text('\n'.join(s)+'\n')


def summarize(root:Path,out:Path,loads=(70,85,95),seeds=(101,102,103),cycles=96):
    out.mkdir(parents=True,exist_ok=True)
    completion=[];timing=[];integrity=[];violations=[]
    fingerprint_groups={}
    for load in loads:
        for seed in seeds:
            for variant in VARIANTS:
                folder=root/f'L{load}'/f'S{seed}'/variant
                proto=folder/'protocol.txt';log=folder/'churn_thrput.txt';status=folder/'exitcode.txt';trace=folder/'paired-traces.csv'
                if not (proto.exists() and status.exists()):
                    violations.append(f'missing protocol/exit code {folder}')
                    continue
                code=int(status.read_text().strip())
                if log.exists():
                    try: record, phases=audit_log(proto.read_text(),log.read_text(),variant,f'L{load}/S{seed}')
                    except Exception as e:
                        violations.append(f'corrupt original log {folder}: {e}')
                        continue
                else:
                    record={'variant':variant,'run_id':f'L{load}/S{seed}','load_pct':load,'cycles':cycles,
                            'completed_cycles':0,'recorded_rows':0,'expected_rows':3*cycles,'complete':False}
                    phases=[]
                complete=bool(record['complete'] and code==0)
                completion.append(dict(load_pct=load,seed=seed,variant=variant,expected_cycles=cycles,
                    recorded_phase_rows=record['recorded_rows'],completed_cycles=record['completed_cycles'],
                    exit_code=code,status='complete' if complete else 'failed_or_incomplete'))
                if trace.exists():
                    with trace.open(newline='') as f: rr=list(csv.DictReader(f))
                    seen=set()
                    for r in rr:
                        k=(r['phase'],int(r['cycle']))
                        if k in seen:violations.append(f'duplicate trace {folder} {k}')
                        seen.add(k)
                        fingerprint_groups.setdefault((load,seed,*k),{})[variant]=(r['operations'],r['fnv64'])
                else:
                    if complete:
                        violations.append(f'absent instrumented trace in complete run {folder}')
                if complete:
                    for phase in PHASES:
                        sample=[float(p['clock_ticks_per_op']) for p in phases
                            if p['phase']==phase and int(p['cycle'])>=cycles-32]
                        if len(sample)!=32:
                            violations.append(f'missing complete trailing cycles {folder} {phase}')
                            continue
                        timing.append(dict(load_pct=load,seed=seed,variant=variant,phase=phase,
                            median_last32_ticks_per_op=f'{statistics.median(sample):.6f}',
                            mean_last32_ticks_per_op=f'{statistics.mean(sample):.6f}'))
    for (load,seed,phase,cycle),v in sorted(fingerprint_groups.items()):
        matched=len(set(v.values()))==1
        if not matched:violations.append(f'TRACE MISMATCH load={load} seed={seed} {phase}:{cycle} {v}')
        integrity.append(dict(load_pct=load,seed=seed,phase=phase,cycle=cycle,
                              represented_variants=len(v),identical=matched,
                              fnv64=next(iter(v.values()))[1]))
    summary=[]
    for load in loads:
        for variant in VARIANTS:
            for phase in PHASES:
                grp=[r for r in timing if r['load_pct']==load and r['variant']==variant and r['phase']==phase]
                if not grp:continue
                samples=[float(r['median_last32_ticks_per_op']) for r in grp]
                summary.append(dict(load_pct=load,variant=variant,phase=phase,
                    completed_seeds=len(samples),median_last32_ticks_per_op=f'{statistics.median(samples):.6f}',
                    mean_of_seed_medians=f'{statistics.mean(samples):.6f}'))
    write_csv(out/'completion.csv',completion,['load_pct','seed','variant','expected_cycles','recorded_phase_rows','completed_cycles','exit_code','status'])
    write_csv(out/'native-complete-timings.csv',timing,['load_pct','seed','variant','phase','median_last32_ticks_per_op','mean_last32_ticks_per_op'])
    write_csv(out/'summary.csv',summary,['load_pct','variant','phase','completed_seeds','median_last32_ticks_per_op','mean_of_seed_medians'])
    write_csv(out/'trace-integrity.csv',integrity,['load_pct','seed','phase','cycle','represented_variants','identical','fnv64'])
    svg_chart(summary,Path('figures/v0.12.0-zombie-native-insert.svg'))
    status={'schema':'zombie-author-source-native-paired-pilot-v0.12.0',
            'source_commit':'5064a381fd3b7a540594fadd6f7595fdcc748a6c',
            'instrumentation':'workload-generator-only; author hash-table implementations unchanged',
            'metric_units':'author clock ticks per operation (NOT ns)',
            'loads':list(loads),'seeds':list(seeds),'cycles':cycles,
            'expected_runs':len(loads)*len(seeds)*len(VARIANTS),
            'runs_with_auditable_completion':len(completion),
            'complete_runs':sum(r['status']=='complete' for r in completion),
            'failed_or_incomplete_runs':sum(r['status']!='complete' for r in completion),
            'matched_trace_rows':len(integrity),'integrity_errors':violations,
            'note':'Nonformal cloud pilot; not uniform-memory L3, no HF-Score.'}
    (out/'provenance.json').write_text(json.dumps(status,indent=2)+'\n')
    with (out/'sha256.txt').open('w') as manifest:
        for name in ('completion.csv','native-complete-timings.csv','summary.csv','trace-integrity.csv'):
            digest=hashlib.sha256((out/name).read_bytes()).hexdigest()
            manifest.write(f'{digest}  {name}\n')
    print(f"Validated {len(completion)}/{status['expected_runs']} runs; {status['complete_runs']} complete, {status['failed_or_incomplete_runs']} incomplete; {len(integrity)} trace fingerprints, {len(violations)} integrity errors")
    return status

if __name__=='__main__':
    p=argparse.ArgumentParser()
    p.add_argument('--root',type=Path,default=Path('native-logs'))
    p.add_argument('--out',type=Path,default=Path('results/v0.12.0-zombie-native'))
    p.add_argument('--cycles',type=int,default=96)
    a=p.parse_args()
    if (a.out/'sha256.txt').exists(): (a.out/'sha256.txt').unlink()
    status=summarize(a.root,a.out,cycles=a.cycles)
    sys.exit(1 if status['integrity_errors'] or status['runs_with_auditable_completion']!=status['expected_runs'] else 0)
