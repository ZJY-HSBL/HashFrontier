# HashFrontier v0.3.0 — 320-cycle exact tails and a negative algorithmic result

Published 2026-10-22. This release is an exploratory comparison, **not** a faithful implementation or confirmed reproduction of the SIGMOD Zombie Hashing paper.

- 8 implementations × 3 occupancies × 6 paired seeds × 320 complete churn cycles = **46,080 independent cycle records**; six seeds, not 46,080 independent statistical repetitions.
- 2.4 million randomized differential oracle operations and adversarial full-table wraparound tests passed on the local build.
- Two bounded-repair controls (8/32 slots) were added to test a falsifiable hypothesis. Both failed to prevent high-load tombstone saturation; long-run misses reached 4,096 probes at 95% occupancy.
- Exact all-cycle update histograms, grouped by algorithm/load/seed (144 groups), measure P50/P95/P99/P99.9 and max probes. Rare full-table rebuilds caused a maximum recorded 107,188 probes even when the P99 was about 1,555.
- 95%-load late-phase misses: Robin Hood 10.34; Backshift 162.00; Bounded 8 and Bounded 32 each 4,096. This is not a universal CPU time ranking.
- Three SVG data visualizations and an auditable 95%-load overview appear in the bilingual README.
- Release automation reruns the full deterministic C++ trace workload on the GitHub runner and attaches raw 46,080-cycle CSV.gz, 144 exact-tail groups and CI runtime metadata.
- An isolated pinned author-native 320-cycle q=13 workflow for RHM/TRHM/GZHM/GZHM_ADAPTIVE is present, but full published q=27 paper reproduction is still pending.

**Next:** establish source-backed native invariants, and preregister independent workloads before assessing any new online deamortization strategy.
