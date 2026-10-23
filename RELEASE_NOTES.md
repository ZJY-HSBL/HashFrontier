# HashFrontier v0.8.1 — First complete three-paper fixed-trace evidence

This revision **supersedes v0.8.0's unsuccessful preflight commit**, which accidentally reverted the validated ZombieHT positive insertion status-code handling. v0.8.1 restores `g_insert >= 0` as required by the author API and adds two distinct, audited experiments:

1. **Dynamic CPU set workloads:** SIGMOD 2025 ZombieHT (GZHM and Adaptive), VLDB 2026 TPHT (Flattened and Chained), SODA 2025 basic Bubble-up §4 with a separately labeled direct-erase extension, and STL baseline. Identical binary operation traces: 70/85/95% loads × ten seeds × 32 mutation/query cycles = **180 attempted runs**. Earlier v0.8.0 GitHub Actions executed 163/180 successfully. All 17 observed failures belong to the basic Bubble-up delete-extended version, not to author-native GZHM/TPHT. The run failures are not proof of a defect in the original paper's insertion-only theory.
2. **Construction-only static workload:** Identical initial key set per seed, followed by 4,096 fixed successful + 4,096 fixed unsuccessful queries, with full SHA-256 provenance and NO delete extension. Another **180 attempted runs**, including the SODA §4 warmup method in its analyzed insertion-only regime.

The release workflow attaches full raw CSV, static and dynamic completion audits, author source SHA and trace hashes, native logs and real SVG charts, and seed-paired *timing-only exploratory* ratios. Results are reported *only* after remote runs complete. This version does **not** claim SODA advanced §5 replication, a publication-grade 30-seed study, or a formal HF-Score (memory and table semantics still require careful alignment).

Original author-core versions remain pinned; the advanced SODA implementation remains pending. The fixed 180-run matrix is deliberately preserved without excluding failed methods.
