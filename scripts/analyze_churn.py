#!/usr/bin/env python3
"""Analyze paired long-churn traces; no performance claim from short hardware timings."""
import csv
import statistics
from collections import defaultdict
from pathlib import Path

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'results/v0.2.0-churn.csv'
OUTPUT = ROOT / 'results/v0.2.0-churn-summary.csv'
FIGURES = ROOT / 'figures'
ALGS = ['linear_tombstone', 'linear_backshift', 'robin_hood',
        'linear_rebuild_2pct', 'linear_rebuild_8pct', 'linear_rebuild_half_slack']
LOADS = (75, 85, 95)
METRICS = ('hit_probes_per_op', 'miss_probes_per_op',
           'update_probes_per_pair', 'p99_update_probes', 'max_update_probes',
           'pair_mean_ns', 'p99_pair_ns', 'max_pair_ns', 'rebuilds', 'moves')


def main():
    rows = list(csv.DictReader(SOURCE.open(newline='', encoding='utf8')))
    for r in rows:
        r['seed'] = int(r['seed'])
        r['cycle'] = int(r['cycle'])
        r['load_pct'] = int(r['load_pct'])
        for metric in METRICS:
            r[metric] = float(r[metric])
    keys = [(r['algorithm'], r['load_pct'], r['seed'], r['cycle']) for r in rows]
    if len(rows) != 6 * 3 * 5 * 12 or len(set(keys)) != len(rows):
        raise ValueError('incomplete/duplicate benchmark data')
    if set(r['algorithm'] for r in rows) != set(ALGS):
        raise ValueError('unexpected algorithm names')
    if set(r['seed'] for r in rows) != set(range(1, 6)):
        raise ValueError('unexpected seed set')
    if set(r['cycle'] for r in rows) != set(range(1, 13)):
        raise ValueError('unexpected cycle set')
    if set(r['load_pct'] for r in rows) != set(LOADS):
        raise ValueError('unexpected load set')
    # One independent observation is a *seed*, with final-four-cycle averaging
    # within seed; do not pseudoreplicate 4 cycles as extra independent trials.
    per_seed = defaultdict(list)
    for r in rows:
        if r['cycle'] >= 9:
            per_seed[(r['algorithm'], r['load_pct'], r['seed'])].append(r)
    records = []
    for name in ALGS:
        for load in LOADS:
            seed_means = []
            for seed in range(1, 6):
                data = per_seed[(name, load, seed)]
                if len(data) != 4:
                    raise ValueError('missing final-cycle row')
                seed_means.append({k: statistics.mean(x[k] for x in data) for k in METRICS})
            row = {'algorithm': name, 'load_pct': load, 'seeds': 5, 'cycles_averaged': '9-12'}
            for key in METRICS:
                vals = [x[key] for x in seed_means]
                row['mean_' + key] = statistics.mean(vals)
                row['sd_seed_' + key] = statistics.stdev(vals)
                # Exploratory t interval (not a strong inferential assertion).
                row['exploratory_ci95_halfwidth_' + key] = 2.776445 * row['sd_seed_' + key] / (5 ** .5)
            records.append(row)
    with OUTPUT.open('w', newline='', encoding='utf8') as f:
        writer = csv.DictWriter(f, fieldnames=list(records[0]))
        writer.writeheader()
        for row in records:
            writer.writerow({k: f'{v:.7g}' if isinstance(v, float) else v for k, v in row.items()})

    FIGURES.mkdir(exist_ok=True)
    labels = {
        'linear_tombstone': 'Tombstones',
        'linear_backshift': 'Backshift',
        'robin_hood': 'Robin Hood',
        'linear_rebuild_2pct': 'Rebuild 2% capacity',
        'linear_rebuild_8pct': 'Rebuild 8% capacity',
        'linear_rebuild_half_slack': 'Rebuild 50% slack',
    }
    fig, ax = plt.subplots(figsize=(9, 5))
    for name in ALGS:
        means = [statistics.mean(r['miss_probes_per_op'] for r in rows
                                 if r['algorithm'] == name and r['load_pct'] == 95 and r['cycle'] == c)
                 for c in range(1, 13)]
        ax.plot(range(1, 13), means, marker='o', markersize=3, label=labels[name])
    ax.set(yscale='log', xlabel='Churn cycle (5% key replacements each)',
           ylabel='Mean negative-lookup slot probes (log scale)',
           title='HashFrontier v0.2.0 | 95% occupancy | paired 5 seeds')
    ax.legend(fontsize=8, ncol=2)
    ax.grid(alpha=.3)
    fig.tight_layout()
    fig.savefig(FIGURES / 'v0.2.0-miss-drift.svg')
    plt.close(fig)

    fig, ax = plt.subplots(figsize=(9, 5))
    for name in ALGS:
        ys = [next(r['mean_p99_update_probes'] for r in records
                   if r['algorithm'] == name and r['load_pct'] == load) for load in LOADS]
        ax.plot(LOADS, ys, marker='o', linewidth=1.7, label=labels[name])
    ax.set(yscale='log', xticks=LOADS, xlabel='Occupancy (%)',
           ylabel='Mean per-cycle P99 update-pair probes (log scale)',
           title='HashFrontier v0.2.0 | last four cycles | no clock-noise confound')
    ax.legend(fontsize=8, ncol=2)
    ax.grid(alpha=.3)
    fig.tight_layout()
    fig.savefig(FIGURES / 'v0.2.0-update-p99.svg')
    plt.close(fig)

    fig, ax = plt.subplots(figsize=(9, 5))
    for name in ALGS:
        vals = [next(r for r in records if r['algorithm'] == name and r['load_pct'] == load)
                for load in LOADS]
        ax.plot([v['mean_miss_probes_per_op'] for v in vals],
                [v['mean_p99_update_probes'] for v in vals],
                marker='o', linewidth=1.7, label=labels[name])
    ax.set(xscale='log', yscale='log',
           xlabel='Mean miss probes per lookup (log scale)',
           ylabel='Mean P99 probes per update pair (log scale)',
           title='HashFrontier v0.2.0 | 75/85/95% occupancy | lower-left preferred')
    ax.legend(fontsize=8, ncol=2)
    ax.grid(alpha=.3)
    fig.tight_layout()
    fig.savefig(FIGURES / 'v0.2.0-tradeoff.svg')
    plt.close(fig)
    print(f'validated {len(rows)} paired records, {len(records)} aggregate groups')
    for r in records:
        if r['load_pct'] == 95:
            print(r['algorithm'], 'miss=', round(r['mean_miss_probes_per_op'], 2),
                  'updateP99=', round(r['mean_p99_update_probes'], 1),
                  'rebuilds/cycle=', round(r['mean_rebuilds'], 2))


if __name__ == '__main__':
    main()
