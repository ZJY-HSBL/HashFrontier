## HashFrontier v0.1.0 — reproducible baseline foundation

- Three classical sequential open-addressing baselines: linear probing with tombstones, linear probing with backward-shift deletion, Robin Hood hashing.
- Release-mode differential test (900,000 randomized operations); CTest and CI smoke benchmark.
- Initial five-paper top-venue registry (2025), all marked **queued**, with a transparent review protocol.
- Same-key paired exploratory CPU benchmark: 5 seeds × 3 loads × 3 distributions × 3 algorithms × 6 phases = **810 measurements**.
- Raw CSV, grouped analysis, SVG figures, bilingual README.

**Evidence caution:** these are local container exploratory microbenchmarks, NOT a replication of any 2025 paper, nor evidence of a new algorithm's superiority. Measurements are platform-dependent and not publication-grade. The release attachment preserves the local exploratory observations; see README for exact conditions.
