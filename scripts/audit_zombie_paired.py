#!/usr/bin/env python3
"""Verify provenance, phase completeness and exact paired author-native traces.

Partial or crashing variants remain in audit output but are excluded from
complete-cohort comparisons. A checksum mismatch *fails* the verification.
"""
from __future__ import annotations
import argparse
from collections import defaultdict
import csv
from pathlib import Path
import statistics
import sys
from audit_zombie_native import audit_log, write_rows

VARIANTS = ('RHM', 'TRHM', 'GZHM', 'GZHM_ADAPTIVE')

def analyze(root: Path, out: Path, loads: tuple[int, ...] = (85, 95), seeds: tuple[int, ...] = (41, 53, 67)):
    out.mkdir(parents=True, exist_ok=True)
    audit, cycles, traces = [], [], defaultdict(dict)
    errors = []
    for load in loads:
        for seed in seeds:
            for variant in VARIANTS:
                directory = root / f'L{load}' / f'S{seed}' / variant
                if not directory.is_dir():
                    errors.append(f'MISSING DIRECTORY {directory}')
                    continue
                proto = (directory / 'protocol.txt').read_text()
                log_file = directory / 'churn_thrput.txt'
                if not log_file.exists():
                    errors.append(f'MISSING PHASE LOG {directory}')
                    continue
                report, rows = audit_log(proto, log_file.read_text(), variant, f'L{load}/S{seed}')
                exitcode = (directory / 'exitcode.txt').read_text().strip()
                report.update(exitcode=int(exitcode), seed=seed,
                              paired_instrumented=True, native_unmodified=False)
                audit.append(report)
                cycles.extend(rows)
                trace_file = directory / 'paired-traces.csv'
                if not trace_file.exists():
                    errors.append(f'MISSING TRACE {directory}')
                    continue
                with trace_file.open(newline='') as stream:
                    rr = list(csv.DictReader(stream))
                seen = set()
                for ix, r in enumerate(rr):
                    k = (r['phase'], int(r['cycle']))
                    if k in seen:
                        errors.append(f'DUPLICATE TRACE {directory} {k}')
                    seen.add(k)
                    if (ix == 0 and k != ('BUILD', -1)) or (ix and k != ('CHURN', ix-1)):
                        errors.append(f'UNEXPECTED TRACE ORDER {directory} {k} index {ix}')
                    traces[(load, seed, k)][variant] = (r['operations'], r['fnv64'])
                if len(rr) != 1 + report['cycles']:
                    # For early abort we still record the generated trace through failing cycle.
                    if report['complete'] or len(rr) < report['completed_cycles'] + 1:
                        errors.append(f'BAD TRACE COUNT {directory} {len(rr)}')
                if load == 85 and (not report['complete'] or int(exitcode) != 0):
                    errors.append(f'85% expected complete; {directory}: cycles={report["completed_cycles"]} exit={exitcode}')
                if load == 95 and variant != 'TRHM' and (not report['complete'] or int(exitcode) != 0):
                    errors.append(f'95% required variant incomplete: {directory}')
    matched = []
    for (load, seed, k), values in sorted(traces.items(),key=lambda x:(x[0][0],x[0][1],x[0][2][1])):
        different = len(set(values.values())) > 1
        if different:
            errors.append(f'TRACE MISMATCH load={load} seed={seed} phase={k}: {values}')
        matched.append(dict(load=load,seed=seed,phase=k[0],cycle=k[1],
                            represented_variants=len(values), identical=(not different),
                            fnv64=next(iter(values.values()))[1]))
    complete = {(int(r['load_pct']),int(r['seed']),r['variant']) for r in audit
                if r['complete'] and r['exitcode'] == 0}
    # Only compare groups where every method finished the exact same 320-cycle tape.
    cohort = defaultdict(list)
    for load in loads:
        for seed in seeds:
            complete_variants = [v for v in VARIANTS if (load,seed,v) in complete]
            if not all(v in complete_variants for v in ('RHM','GZHM','GZHM_ADAPTIVE')):
                continue
            for variant in complete_variants:
                observations = [r for r in cycles
                                if r['variant']==variant and r['run_id']==f'L{load}/S{seed}'
                                and int(r['cycle'])>=256]
                for phase in ('DELETE','INSERT','LOOKUP'):
                    vals = [float(r['clock_ticks_per_op']) for r in observations if r['phase']==phase]
                    if len(vals)!=64:
                        errors.append(f'BAD FINISHER {load} {seed} {variant} {phase} {len(vals)}')
                        continue
                    cohort[(load,variant,phase)].append((seed,statistics.median(vals)))
    summary = []
    for (load,variant,phase),vals in sorted(cohort.items()):
        summary.append(dict(load=load,variant=variant,phase=phase,
                            complete_seeds=len(vals),seeds=';'.join(str(s) for s,_ in vals),
                            median_of_final64_cycle_medians=round(statistics.median(v for _,v in vals),3),
                            mean_of_seed_medians=round(statistics.mean(v for _,v in vals),3)))
    if audit:write_rows(out/'completion.csv',audit)
    if matched:write_rows(out/'trace-integrity.csv',matched)
    if summary:write_rows(out/'complete-cohorts.csv',summary)
    if cycles:write_rows(out/'native-cycles.csv',cycles)
    (out/'errors.txt').write_text('\n'.join(errors) + ('\n' if errors else ''),encoding='utf8')
    print(f'Input audit: {len(audit)} runs, {len(traces)} tape fingerprints, {len(cycles)} complete cycle-phase records, {len(errors)} errors')
    for row in audit:
        print(f"  {row['run_id']}/{row['variant']} {row['completed_cycles']}/{row['cycles']} exit={row['exitcode']}")
    for e in errors[:25]:print('ERROR',e,file=sys.stderr)
    return 1 if errors else 0

if __name__ == '__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('input',type=Path)
    parser.add_argument('--out',type=Path,default=Path('audit'))
    args=parser.parse_args()
    raise SystemExit(analyze(args.input,args.out))
