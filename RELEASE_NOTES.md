# HashFrontier v0.9.1 — SODA 2025 Appendix C tombstone and rebuild mechanism

- Adds a separate **SODA 2025 Appendix C** tombstone implementation on top of the independent §5.2 finite-parameter insertion engine. Physical tombstones retain their marker through eviction, reinsertion unmarks, and complete rehash excludes tombstones at augmented-load threshold.
- Correctness: 2,447,592 random oracle/property checks across 54 parameter/seed cohorts, 888 observed full rebuilds, ASan/UBSan passed locally. Original §5.2 regression still passes.
- Real pilot: 30/30 dynamic traces, 70/85/95% live load × 10 seeds × 28 churn cycles. Reports distinct insert/delete/hit/miss latencies, P99, maximum updates, rebuild counts, physically occupied entries, and **implementation-slot-only** bytes (not formal memory). Full CSV and two real SVG figures included.
- Explicit scientific boundary: this is **L1 mechanism-level** validation only. At n=4096 and d_core=5, the admissible ε interval of Theorem 5.1 is empty; d=10 does not satisfy the theorem's minimal-choice specification. No original-paper L2 figure or cross-paper L3/HF-Score is claimed.
- Publishes strict L2/L3 audit matrix for ZombieHT and TPHT, plus an independent GitHub Actions re-run of the Appendix C finite test. Remains in paper reproduction scope; no original algorithm.
