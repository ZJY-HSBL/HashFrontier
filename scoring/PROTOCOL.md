# HF-Score v1.0 / 散列表统一评价协议

**Status: preregistered method, not a measured cross-paper leaderboard.**

**适用赛道：**单线程 CPU 动态精确字典；静态最小完美散列、并发及 GPU 单独制定评分协议。不能将功能不同的算法放在同一排名。

## Seven fixed lower-is-better metrics

| Raw column | Unit | Weight | Definition |
|---|---|---:|---|
| `build_ns_per_key` | ns/key | 0.15 | Wall-clock initial construction, no IO or trace generation |
| `hit_ns_per_op` | ns/operation | 0.20 | Successful lookup on prerecorded hit stream |
| `miss_ns_per_op` | ns/operation | 0.15 | Unsuccessful lookup on prerecorded miss stream |
| `insert_ns_per_op` | ns/operation | 0.15 | Distinct new-key insertion; required relocation included |
| `delete_ns_per_op` | ns/operation | 0.10 | Successful key deletion and any mandatory maintenance |
| `update_p99_ns` | ns/operation | 0.10 | Nearest-rank P99 of **individual mutation operations**, including periodic/rehash pauses |
| `bytes_per_key` | bytes/key | 0.15 | Full live-table allocation including control metadata, over number of live entries; report collection method |

A shared `uint64_t -> uint64_t` key/value interface, exact set/dictionary semantics and **no loss of keys or values** are mandatory. Correctness is a hard gate, **not** a tradeable accuracy score. Each algorithm must meet workload and capacity constraints; errors, hangs, insufficient memory or unimplemented operations are recorded as failed/unsupported, **never removed from the denominator**. Use `status=complete`, `correct=true` only with a completed differential oracle and all seven strictly positive finite measurements; other statuses carry empty metric fields.

The **measurement contract** is: same hardware/compiler configuration, same initial keys, identical replayed operations and phase order for a given scenario/seed, same payload width, same mutation sampling/timeout/timeout boundary, identical warmup policy. For differing legal hash families (e.g. d-ary cuckoo), preserve paper-specified hashing but feed exactly the same key/operation streams. No reshuffling per implementation. `trace_sha256` must hash the complete serialized scenario-specific operation trace, including initial keys, mutation order and query requests. A supplied hash without actually replaying the trace is not enough; record generator+source SHA externally. Record CPU affinity, clock policy, compilation flags, allocator, RSS and/or allocator measurement methodology in the experiment manifest. For load factors impossible for a method, mark `unsupported`; do not quietly lower the load.

**Metric separation:** `build_ns_per_key` and `insert_ns_per_op` measure *different phases*; `update_p99_ns` pools mutation-operation latencies over the same prerecorded update phase and uses the raw per-operation distribution. Do not compute a weighted average of P99 values or infer ns/op from probe counts. Memory must be complete allocations, not merely the number of slots in a vector. Absent measured metrics cannot be inferred from other columns.

## Frozen score

For algorithm `a`, scenario `s`, seed `i`, and metric `j`, define the logarithmic paired gain

$$
g_{a,s,i}=\sum_j w_j\log\!\left(\frac{x_{b,s,i,j}}{x_{a,s,i,j}}\right),
$$

where `b` is frozen reference `robin_hood`, all `x` are strictly positive lower-is-better metrics, `w` are the seven fixed weights. Pairing is at the identical scenario, seed, hardware and SHA-256 trace. Define the per-scenario gain `z(a,s)` as the arithmetic mean of `g(a,s,i)` across **all** preregistered 30 seeds. Then, with frozen scenario weights `p_s`:

$$
\mu_a=\sum_s p_s z(a,s), \quad
\sigma_a=\sqrt{\sum_s p_s(z(a,s)-\mu_a)^2}, \quad
\operatorname{HFScore}(a)=100\exp(\mu_a-0.25\sigma_a).
$$

Thus the baseline is 100 exactly; >100 means *risk-adjusted multiplicative improvement* over that reference, **not** a percentage accuracy score. `σ` measures **cross-scenario variation**, not statistical uncertainty across random seeds. Bootstrapped 95% confidence intervals use paired seed-vector resampling across all scenarios (1000 draws, fixed RNG seed); 30 seeds are the minimum for a *formal candidate* score. Statistical significance and claims of superiority still require separate multiplicity-aware paired hypothesis tests; score CI alone does not prove publishable superiority.

## Input / output and strict gates

- `scoring/protocol-v1.0.json` freezes metric weights, scenario weights, lambda, expected algorithms, reference and seeds. Its placeholder hardware ID **must be replaced with a real frozen manifest** before an experiment. The four scenarios in this JSON are a **pilot scoring profile** and must not be presented as exhaustive workload coverage. Publication-scale preregistration should expand to the full planned scenario matrix **before opening results**, with a new versioned manifest. Modifying the benchmark suite/weights requires a versioned protocol change, not post hoc adjustment.
- CSV columns: `algorithm,scenario,seed,track,hardware_id,trace_sha256,status,correct,build_ns_per_key,hit_ns_per_op,miss_ns_per_op,insert_ns_per_op,delete_ns_per_op,update_p99_ns,bytes_per_key`.
- One row per `(algorithm,scenario,seed)`. An absent row or an incomplete run makes that algorithm **ineligible**; missing reference runs invalidate the entire file. Duplicate rows, mismatched complete-trace SHA, incompatible hardware or malformed metrics stop scoring. No partial-seven-metric score is emitted.
- `evidence_level=formal_candidate` only with at least 30 preregistered seeds and `dataset_kind=measurement`. For fewer seeds mark `exploratory_insufficient_seeds`. Test fixtures mark `synthetic_fixture_not_evidence`, and must never be presented as real experiments.

```bash
python3 scoring/hf_score.py \
  --protocol scoring/protocol-v1.0.json \
  --observations path/to/real_cross_paper_measurements.csv \
  --output results/hf-score-v1.0.json
python3 -m unittest discover -s tests -p 'test_hf_score.py' -v
```

**Current evidence:** v0.1.0 collected combined insert/delete churn timing, vector-slot-only memory, and no per-mutation nanosecond P99; v0.2.0–v0.5.0 similarly lack the complete seven-metric matched *cross-paper* collection. All are excluded from HF-Score. The current repository has **no valid formal cross-paper HF-Score ranking**. Complete that measurement first; then generate tables, scenario Pareto plots, weight-sensitivity analysis and independent held-out confirmation. Report separate success/failure-query, tail and memory results alongside any composite score.
