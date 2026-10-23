# v0.10.0 L1/L2/L3 verification ledger / 论文复现验收

Only faithfully reproduced eligible 2025–2026 hash-table papers belong to this project; **no algorithm innovation in this phase**.

| Paper | L1 evidence | L2 paper-native | L3 frozen CPU comparison | Exact blocker |
|---|---|---|---|---|
| SODA 2025 Bubble-up | §4, §5.2, Appendix C mechanisms and randomized tests | **incomplete** | **incomplete** | Appendix B and theory conditions/fully independent hashes; numerical theorem preconditions fail |
| SIGMOD 2025 Zombie Hashing | Pinned author RHM/TRHM/GZHM/Adaptive; scaled traces | **incomplete** | **incomplete** | Full author q=27 load/workload and all published configurations; preserve high-load TRHM failures |
| PVLDB 2026 TPHT | Author Chained/Flattened source tests, native latency and space sweeps | **partial** | **incomplete** | Full publication figures and original parameter match; 64-bit values and allocator accounting |

**Do not retroactively relabel 2026-10-23 Debian or GitHub Actions CPU timing as target-host results.** CPU target is specified, **not physically bound**. When bound, all papers are rerun on the same fixed physical host with unchanged 30 paired seed traces, original source/params and full seven metrics; only then assess formal HF-Score. No new research paper advances to formal L3 before existing work has a valid author/paper-faithful gate.
