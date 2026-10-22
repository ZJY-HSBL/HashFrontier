#!/usr/bin/env python3
"""Read real benchmark CSV; produce grouped summaries and versioned SVG figures."""
import argparse
import csv
import math
import statistics
from collections import defaultdict
from pathlib import Path

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('csv', type=Path)
    ap.add_argument('--out', type=Path, default=Path('figures'))
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    records = list(csv.DictReader(args.csv.open(newline='', encoding='utf8')))
    for r in records:
        for k in ('seed', 'capacity', 'size', 'operations', 'elapsed_ns', 'allocated_bytes'):
            r[k] = int(r[k])
        for k in ('load', 'ns_per_op', 'probes_per_op', 'moves_per_op'):
            r[k] = float(r[k])
    data = defaultdict(list)
    for r in records:
        data[(r['algorithm'], r['pattern'], r['load'], r['phase'])].append(r)
    expected = 3 * 3 * 3 * 5 * 6
    if len(records) != expected: raise ValueError(f'expected {expected} observations, got {len(records)}')
    if any(len(v) != 5 for v in data.values()): raise ValueError('missing seeds or duplicates')
    summary = args.out.parent / 'results' / 'v0.1.0-summary.csv'
    summary.parent.mkdir(exist_ok=True)
    with summary.open('w', newline='', encoding='utf8') as f:
        fields = ['algorithm', 'pattern', 'load', 'phase', 'seeds', 'mean_ns_per_op',
                  'mean_probes_per_op', 'mean_moves_per_op', 'stddev_ns_per_op']
        out = csv.DictWriter(f, fieldnames=fields)
        out.writeheader()
        for (alg, pattern, alpha, phase), rows in sorted(data.items()):
            t = [r['ns_per_op'] for r in rows]
            out.writerow(dict(algorithm=alg, pattern=pattern, load=alpha, phase=phase,
                              seeds=len(rows), mean_ns_per_op=f'{statistics.mean(t):.6g}',
                              mean_probes_per_op=f'{statistics.mean(r["probes_per_op"] for r in rows):.6g}',
                              mean_moves_per_op=f'{statistics.mean(r["moves_per_op"] for r in rows):.6g}',
                              stddev_ns_per_op=f'{statistics.stdev(t):.6g}'))
    methods = [('linear_tombstone','Linear + tombstones'), ('linear_backshift','Linear + backshift'),
               ('robin_hood','Robin Hood')]
    for phase, metric, ylabel, file in (
        ('hit', 'probes_per_op', 'Successful lookup: mean slot probes', 'successful_probes.svg'),
        ('post_miss', 'probes_per_op', 'Post-churn miss: mean slot probes', 'post_churn_misses.svg'),
        ('churn', 'ns_per_op', 'Churn: nanoseconds / operation', 'churn_latency.svg'),
    ):
        fig, ax = plt.subplots(figsize=(8.2, 4.2), dpi=120)
        for alg, label in methods:
            ys = [statistics.mean(r[metric] for r in data[(alg, 'uniform', alpha, phase)])
                  for alpha in (0.5, 0.8, 0.93)]
            ax.plot([50,80,93], ys, marker='o', linewidth=2, label=label)
        ax.set_xticks([50,80,93]); ax.set_xlabel('Occupancy (%)')
        ax.set_ylabel(ylabel); ax.set_title('HashFrontier v0.1.0 | uniform queries | exploratory')
        ax.legend(); ax.grid(alpha=.24); fig.tight_layout()
        fig.savefig(args.out / file, format='svg')
        plt.close(fig)
    print(f'{len(records)} measurements; summary={summary}; SVG figures={args.out}')

if __name__ == '__main__': main()
