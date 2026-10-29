#!/usr/bin/env python3
"""Strict 300-row audit and zero-dependency SVG/CSV report, DMPP06 Transformation 4."""
import csv
import json
import math
import statistics
import sys
from collections import defaultdict
from pathlib import Path
from xml.sax.saxutils import escape

FIELDS = ['method', 'keys', 'bucket_bits', 'distribution', 'seed', 'collision_keys', 'over_threshold_keys', 'max_bucket_load', 'metadata_bytes', 'hash_ns_per_key', 'valid']
METHODS = ('transformation3', 'transformation4')
NS = (4096, 8192, 16384)
BITS = (9, 10, 11, 12, 13)
PROFILES = ('contiguous', 'uniform')
SEEDS = (1, 2, 3, 4, 5)

def load(path):
    with open(path, newline='', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        if reader.fieldnames != FIELDS:
            raise ValueError('Header mismatch')
        rows = list(reader)
    expected = {(m, n, bits, d, seed) for m in METHODS for n in NS for bits in BITS for d in PROFILES for seed in SEEDS}
    if len(rows) != len(expected):
        raise ValueError(f'expected {len(expected)} rows, got {len(rows)}')
    seen = set()
    for row in rows:
        k = (row['method'], int(row['keys']), int(row['bucket_bits']), row['distribution'], int(row['seed']))
        if k not in expected or k in seen:
            raise ValueError(f'unknown/duplicate configuration {k}')
        seen.add(k)
        n, bits = k[1:3]
        collisions, overflow, mx, meta = (int(row[name]) for name in ('collision_keys', 'over_threshold_keys', 'max_bucket_load', 'metadata_bytes'))
        latency = float(row['hash_ns_per_key'])
        if row['valid'] != '1' or not all(0 <= v <= n for v in (collisions, overflow, mx)) or not (meta in (1024, 1536)) or not math.isfinite(latency) or latency <= 0:
            raise ValueError(f'invalid numeric observation {k}')
        if meta != (1024 if k[0] == METHODS[0] else 1536):
            raise ValueError(f'unexpected method metadata {k}')
        if mx < (n + (1 << bits) - 1) // (1 << bits):
            raise ValueError(f'impossible maximum load {k}')
    return rows

def svg_bar(points, title, path, ylabel):
    width, height = 1020, 430
    vmax = max(v for _, v, _ in points) * 1.15 if points else 1
    out = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
           '<rect width="1020" height="430" fill="#f8fafc"/>',
           f'<text x="30" y="36" font-family="sans-serif" font-size="23" font-weight="bold" fill="#0f172a">{escape(title)}</text>',
           f'<text x="30" y="60" font-family="sans-serif" font-size="13" fill="#475569">{escape(ylabel)} · independently verified five-seed medians</text>']
    left, top, plotw, ploth = 55, 85, 930, 280
    out.append(f'<path d="M {left} {top} V {top+ploth} H {left+plotw}" fill="none" stroke="#64748b" stroke-width="1.5"/>')
    bw = plotw / max(1, len(points))
    for i, (label, value, color) in enumerate(points):
        h = max(0, ploth * value / vmax)
        x = left + i * bw + bw * 0.18
        y = top + ploth - h
        out.extend([f'<rect x="{x:.1f}" y="{y:.1f}" width="{bw*.64:.1f}" height="{h:.1f}" rx="3" fill="{color}"/>',
                    f'<text x="{x+bw*.32:.1f}" y="{y-8:.1f}" text-anchor="middle" font-family="sans-serif" font-size="14" fill="#0f172a">{value:.2f}</text>',
                    f'<text x="{x+bw*.32:.1f}" y="{top+ploth+24}" text-anchor="middle" font-family="sans-serif" font-size="12" fill="#334155">{escape(label)}</text>'])
    out.append('</svg>')
    path.write_text('\n'.join(out), encoding='utf-8')

def report(inp, out):
    rows = load(inp)
    out = Path(out)
    out.mkdir(parents=True, exist_ok=True)
    groups = defaultdict(list)
    for r in rows:
        groups[(r['method'], int(r['keys']), int(r['bucket_bits']), r['distribution'])].append(r)
    cols = ['method', 'keys', 'bucket_bits', 'distribution', 'seeds', 'median_collision_keys', 'median_over_threshold_keys', 'median_max_bucket_load', 'median_hash_ns_per_key', 'metadata_bytes', 'b_lt_n']
    summary = []
    for (method, n, bits, profile), values in sorted(groups.items()):
        summary.append(dict(zip(cols, (method, n, bits, profile, len(values),
            statistics.median(int(r['collision_keys']) for r in values),
            statistics.median(int(r['over_threshold_keys']) for r in values),
            statistics.median(int(r['max_bucket_load']) for r in values),
            statistics.median(float(r['hash_ns_per_key']) for r in values),
            int(values[0]['metadata_bytes']), int((1 << bits) < n)))))
    with (out / 'summary.csv').open('w', newline='', encoding='utf-8') as f:
        w = csv.DictWriter(f, fieldnames=cols)
        w.writeheader();w.writerows(summary)
    subset = {r['method']: r for r in summary if r['keys'] == 8192 and r['bucket_bits'] == 10 and r['distribution'] == 'uniform'}
    if set(subset) != set(METHODS):
        raise ValueError('missing strictly paired n8192/b1024 subset')
    palette = {'transformation3': '#2563eb', 'transformation4': '#0d9488'}
    svg_bar([(m, subset[m]['median_collision_keys'], palette[m]) for m in METHODS],
            'DMPP06 16-bit grouped permutation — collisions', out/'collisions.svg', 'n=8192, b=1024, random input')
    svg_bar([(m, subset[m]['median_hash_ns_per_key'], palette[m]) for m in METHODS],
            'DMPP06 16-bit grouped permutation — hashing latency', out/'latency.svg', 'ns/key, n=8192, b=1024')
    audit = {'valid': True, 'raw_rows': len(rows), 'configurations': len(summary), 'seeds_per_configuration': 5,
             'strictly_b_lt_n_rows': sum((1 << int(r['bucket_bits'])) < int(r['keys']) for r in rows),
             'single_machine_comparison_only': True, 'theorem5_claimed': False,
             'siegel_high_independence_implemented': False, 'finite_transformation4_only': True,
             'subset': subset}
    (out/'audit.json').write_text(json.dumps(audit, indent=2, ensure_ascii=False)+'\n',encoding='utf-8')
    print(json.dumps(audit, ensure_ascii=False))

if __name__ == '__main__':
    if len(sys.argv) != 3:raise SystemExit('usage: report_dmpp06_group_v024.py RAW.csv OUT_DIR')
    report(sys.argv[1], sys.argv[2])
