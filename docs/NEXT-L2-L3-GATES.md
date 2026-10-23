# Mandatory reproduction evidence matrix / 逐篇 L2-L3 验收矩阵

**Scope locked:** Published 2025–2026 hash-table algorithms only; no original algorithm invention. A green CI task and a successful small dataset do not constitute complete paper replication.

| Order | Paper / version | Existing evidence | L2 acceptance still missing | L3 acceptance still missing | Gate |
|---|---|---|---|---|---|
| 1 | SODA 2025 Bubble-up §4, §5.2 + Appendix C | Mechanism implementations, invariants, 30 finite-parameter paired build/query cohorts, 30 Appendix C dynamic runs | Theorem-admissible parameter design and paper-setting outcomes; review finite implementation deviations and Appendix C exact asymptotic constants | 64/64 exact dictionary semantics vs dynamic peers, full 7 metrics, 30 matched seeds, common memory accounting | **L1 mechanism only** |
| 2 | SIGMOD 2025 ZombieHT RHM/TRHM/GZHM/Adaptive; authors' `5064a381fd3b7a540594fadd6f7595fdcc748a6c` | Original source compiled; scaled native and binary-trace comparisons; TRHM high-load crash/incomplete cohorts preserved | Paper's **q=27** native experiments, 75/85/95% load, 5/95 and 50/50 update/query, ordered 2000 and unordered 320 cycles where required; native figures and hardware RAM; inspect failed TRHM runs | 64/64 payload, normal and negative lookup semantics, 30 pairs, exact allocator bytes+peak RSS, same CPU/hardware | **L2 incomplete; L3 incomplete** |
| 3 | PVLDB 2026 TPHT Flatten/Chained; authors' `3c39de26faba11bc1a037a1dfd24e3c633ac8987` | Public C source, unified 64-bit key with 1-byte dummy value pilot | Run upstream test suite; **original** latency and space-efficiency driver, original dataset and dimensions, compare official figures; preserve native source commit | 64/64 payload, allocator-instrumented true bytes/key, 30 paired seeds and all seven measures; same capacity semantics | **L2 incomplete; L3 incomplete** |

**Source references:** [ZombieHT](https://github.com/saltsystemslab/ZombieHT); [TPHT](https://github.com/Xilinion/TPHT); [Bubble-up paper](https://doi.org/10.1137/1.9781611978322.133).

### Execution next

1. Finish the publication-specific L2 tasks in the order above. ZombieHT q=27 is ~134 million slots and **requires a runner with enough RAM**; do not silently replace with q=13 and label L2.
2. For TPHT, its actual publisher/author source has distinct value widths; the `value_size=1` set pilot is **not** the 64-bit dictionary score profile. Report that mismatch until the 64-bit-run work is complete.
3. Only after correct L2 datasets exist, rerun *all* eligible dynamic algorithms on frozen 30-seed common 64-bit key/value traces, record full allocation bytes, evaluate the frozen HF-Score protocol. Do not compute from incompatible historical measurements.
4. Move to already-screened [2025–2026 eligible list](../papers/registry-v0.9.0.csv) in sequence: STOC 2025 non-oblivious open addressing → STOC 2026 Greedy Open Addressing → FOCS 2026 accepted variants, with publication status explicitly checked. Static/GPU/concurrent papers remain separate tracks.
