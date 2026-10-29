# HashFrontier v0.26.0 — Siegel 2004 Lemma 6 finite tensor graph mechanism

- Implements Siegel Lemma 6 finite product adjacency from an already k-certified base graph without storing expanded-product edges permanently. Explicit expansion comparator shares exactly the same per-product-right-side random table.
- 4,893,162 full finite unique-neighbor subset checks and exact small 2-bit joint-law tests; ASan/UBSan and 7/7 strict CSV-audit regression tests pass.
- [Independent cloud experiment](https://github.com/ZJY-HSBL/HashFrontier/actions/runs/40850221365): 100/100 paired rows (50 cases, 10 groups, five seeds), all checksums equal, Ubuntu 24.04, AMD EPYC 7763, GCC 13.3.
- Representative u=6,d=3,p=3: stored **edge payload only** 36 vs 11,664 bytes; common right-side random table 13,824 bytes; timed median 60.0498 vs 15.4299 ns/op (factorized vs fully expanded).
- **Not** the full Siegel high-independence explicit graph generator or its asymptotic constant-time/space bounds. Seeded SplitMix64 samples are not a proof of independence. No DMPP06 or STOC 2025 completed, no HF-Score.
