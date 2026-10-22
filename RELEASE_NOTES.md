# HashFrontier v0.2.0 — Churn maintenance tradeoffs and ZombieHT provenance

**Date:** 2026-10-22. **Scope:** verified *classical controls and reference preparation*, **not a completed SIGMOD paper replication**.

- 3 newly implemented tombstone periodic full-rebuild controls: 2% of capacity, 8% of capacity, 50% of available slack. They are deliberately **not** labeled Zombie Hashing.
- Full correctness regression extended to 6 variants × 4 capacities × 6 seeds × 12,500 randomized oracle-checked operations = **1.8 million operations** (the earlier 1.5 million was before the slack control was added).
- 1,080 real long-churn data rows: 6 variants × 3 loads (75%, 85%, 95%) × 5 paired seeds × 12 cycles; each cycle replaces 5% of live keys and executes paired hit/miss queries.
- Records include update-pair P99 and maximum slot probes, absolute rebuild count, relocation cost and exploratory clock measurements. The figures and summary CSV are generated from raw measurements.
- 95%-load final-four-cycle means for negative queries: Robin Hood 10.51 probes; tombstones 2593.27; 2%-capacity rebuild 225.00; 8%-capacity rebuild 2593.27; 50%-slack rebuild 263.22. The 2%-capacity variant incurred mean max update-pair probe cost ~39,031 (averaged across cycles and seeds) from full maintenance.
- Verified original SIGMOD author repository, BSD-3 license, pinned source commit `5064a381fd3b7a540594fadd6f7595fdcc748a6c`, original `GZHM` and `TRHM` build variants, and authored a separate GitHub Actions small-scale source smoke. This is a **reproduction staging gate**, not publication-grade reproduction.

**Interpretation:** fixed tombstone cleanup triggers measured against total capacity can be impossible to reach at high occupancy; slack-relative cleanup lowers misses but periodic rebuilding incurs rare heavy updates. Deamortized window-locality methods warrant an author-faithful test. P99 may miss events occurring less often than 1%; retain maxima and full operation traces. Single-host microbenchmarks and five seeds do not establish universal wall-clock superiority.

**Follow-up:** source-backed ZombieHT native baseline and 95%-load paper protocol, then unify operations, evaluate cycle trajectories, relocation and peak memory, and only thereafter test a new bounded-rearrangement mechanism against genuine 2025 baselines.
