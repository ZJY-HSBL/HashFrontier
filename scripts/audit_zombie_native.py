#!/usr/bin/env python3
"""Audit completeness of the SIGMOD 2025 ZombieHT author-native churn logs.

Reads an upstream GitHub Actions artifact ZIP or a directory containing protocol.txt
and churn_thrput.txt under each variant. A process exit code of 0 is NOT sufficient:
upstream run_churn stops on QF_NO_SPACE but its main() still returns zero.

The output is descriptive native-run evidence, NOT paired HashFrontier benchmarking.
"""
from __future__ import annotations

import argparse
import csv
import io
import json
from pathlib import Path
import sys
import zipfile

PHASES = ('DELETE', 'INSERT', 'LOOKUP')
COLUMNS = ('churn_cycle', 'ts', 'duration', 'num_ops', 'false_lookups',
           'total_lookups', 'false_delete', 'total_delete', 'total_inserts', 'op')


def parse_protocol(raw: str) -> dict:
    config = {}
    for entry in raw.strip().split(';'):
        if '=' in entry:
            key, value = entry.split('=', 1)
            config[key.strip()] = value.strip()
    def first(*keys):
        for key in keys:
            if key in config:
                return int(config[key])
        raise ValueError(f'missing {keys} in protocol {config}')
    return {'load_pct': first('load'), 'cycles': first('cycles'),
            'deletions': first('delete', 'deletions'),
            'insertions': first('insert', 'insertions'),
            'lookups': first('lookups'), 'unpaired': config.get('unpaired', 'true')}


def audit_log(protocol: str, raw_log: str, variant: str, run_id: str) -> tuple[dict, list[dict]]:
    config = parse_protocol(protocol)
    reader = csv.DictReader(io.StringIO(raw_log), delimiter=' ', skipinitialspace=True)
    # DictReader with delimiter=' ' creates extra blank columns from repeated spaces.
    # Normalize manually instead and reject malformed rows rather than silently dropping data.
    lines = [line.strip() for line in raw_log.splitlines() if line.strip()]
    if not lines or tuple(lines[0].split()) != COLUMNS:
        raise ValueError(f'{run_id}/{variant}: unexpected upstream header')
    rows = []
    for line in lines[1:]:
        cells = line.split()
        if len(cells) != len(COLUMNS):
            raise ValueError(f'{run_id}/{variant}: bad line {line!r}')
        r = dict(zip(COLUMNS, cells))
        for key in COLUMNS[:-1]:
            r[key] = int(r[key])
        rows.append(r)
    steps = {'DELETE': config['deletions'], 'INSERT': config['insertions'],
             'LOOKUP': config['lookups']}
    if len(rows) > config['cycles'] * 3:
        raise ValueError(f'{run_id}/{variant}: too many rows')
    for ix, row in enumerate(rows):
        expected_cycle, phase = divmod(ix, 3)
        expected_phase = PHASES[phase]
        if (row['churn_cycle'] != expected_cycle or row['op'] != expected_phase or
                row['num_ops'] != steps[expected_phase] or row['duration'] < 0):
            raise ValueError(f'{run_id}/{variant}: noncanonical or corrupted row {ix}: {row}')
    complete = len(rows) == config['cycles'] * 3
    full = len(rows) // 3
    report = {
        'variant': variant, 'run_id': run_id, **config,
        'recorded_rows': len(rows), 'expected_rows': config['cycles'] * 3,
        'completed_cycles': full, 'complete': complete,
        'trailing_partial_phases': len(rows) % 3,
        'stopped_during': PHASES[len(rows) % 3] if not complete else '',
        'failure_type': 'incomplete_author_log' if not complete else '',
    }
    outputs = []
    for ix in range(full):
        cycle = rows[3 * ix:3 * ix + 3]
        for row in cycle:
            outputs.append({
                'run_id': run_id, 'variant': variant,
                'load_pct': config['load_pct'], 'cycle': ix,
                'phase': row['op'], 'operations': row['num_ops'],
                'duration_clock_ticks': row['duration'],
                'clock_ticks_per_op': round(row['duration'] / row['num_ops'], 6),
                'cumulative_false_lookups': row['false_lookups'],
                'cumulative_false_deletes': row['false_delete'],
                'complete_run': complete,
                'unpaired_trace': config['unpaired'].lower() != 'false',
            })
    return report, outputs


def read_native(path: Path) -> list[tuple[str, str, str, str]]:
    found = []
    if path.is_file():
        with zipfile.ZipFile(path) as zf:
            names = set(zf.namelist())
            for name in sorted(n for n in names if n.endswith('churn_thrput.txt')):
                folder = str(Path(name).parent)
                protocol_name = f'{folder}/protocol.txt'
                if protocol_name not in names:
                    raise ValueError(f'missing {protocol_name}')
                variant = Path(folder).name
                run_id = str(Path(folder).parent) if '/' in folder else 'v03-native'
                found.append((variant, run_id, zf.read(protocol_name).decode(),
                              zf.read(name).decode()))
    else:
        for log_path in sorted(path.rglob('churn_thrput.txt')):
            folder = log_path.parent
            protocol = folder / 'protocol.txt'
            if not protocol.exists():
                raise ValueError(f'missing {protocol}')
            variant = folder.name
            run_id = str(folder.parent.relative_to(path))
            found.append((variant, run_id, protocol.read_text(), log_path.read_text()))
    if not found:
        raise ValueError(f'no original author churn logs in {path}')
    return found


def write_rows(path: Path, rows: list[dict]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if not rows:
        raise ValueError('empty CSV output')
    with path.open('w', newline='', encoding='utf8') as dest:
        w = csv.DictWriter(dest, fieldnames=list(rows[0]))
        w.writeheader()
        w.writerows(rows)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('input', type=Path, help='author artifact ZIP or native-logs directory')
    ap.add_argument('--out', type=Path, default=Path('results/zombie-audit'))
    ap.add_argument('--allow-incomplete', nargs='*', default=[],
                    help='Explicitly documented expected incomplete variants, e.g. TRHM')
    ap.add_argument('--require-complete', nargs='*', default=[],
                    help='Variants that MUST complete in every run, e.g. RHM GZHM GZHM_ADAPTIVE')
    args = ap.parse_args(argv)
    reports, cycles = [], []
    for variant, run_id, proto, log in read_native(args.input):
        report, rows = audit_log(proto, log, variant, run_id)
        reports.append(report)
        cycles.extend(rows)
    seen = {r['variant'] for r in reports}
    missing = set(args.require_complete) - seen
    unexpected = [r for r in reports if not r['complete'] and
                  (r['variant'] not in args.allow_incomplete or r['variant'] in args.require_complete)]
    if missing:
        print(f'ERROR missing required variants: {sorted(missing)}', file=sys.stderr)
    args.out.mkdir(parents=True, exist_ok=True)
    write_rows(args.out / 'audit.csv', reports)
    write_rows(args.out / 'native-cycles.csv', cycles)
    (args.out / 'audit.json').write_text(json.dumps(reports, indent=2), encoding='utf8')
    for row in reports:
        status = 'COMPLETE' if row['complete'] else 'INCOMPLETE'
        print(f"{row['run_id']}/{row['variant']} load={row['load_pct']}% "
              f"{row['completed_cycles']}/{row['cycles']} cycles ({status}); "
              f"rows={row['recorded_rows']}/{row['expected_rows']}")
    if missing or unexpected:
        print('ERROR: incomplete/absent author-native experiment; see audit.csv', file=sys.stderr)
        return 3
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
