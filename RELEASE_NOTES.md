# HashFrontier v0.4.0 — Auditing incomplete original ZombieHT experiments

**2026-10-22 | SIGMOD 2025 Zombie Hashing author-source audit | Evidence, not a new algorithm**

## Critical correction to v0.3.0

The author's pinned source and CI job completed with a successful process exit, but inspecting the *original upstream per-phase output* shows that only **RHM, GZHM, GZHM_ADAPTIVE** ran the requested 320 full cycles at 95% occupancy, q=13. **TRHM completed 194/320 cycles and then logged only DELETE in cycle 194 (zero-based); it did not complete INSERT or LOOKUP.** 583 phase records appeared instead of 960. This run must not be labeled a successful full 320-cycle TRHM experiment.

The upstream `bench/hm_churn.cc::profile_ops()` returns `-1` on `QF_NO_SPACE`; `run_churn()` breaks its loop, but `main()` still returns `0`. Thus GitHub Actions green is not evidence of cycle completeness. This is a fidelity/correctness-of-experimental-reporting finding, not proof the Zombie algorithm is wrong. Confirming the reason for this exact TRHM stop still requires an instrumented rerun.

## Deliverables

- Recovered genuine GitHub Actions artifact ID 12432115740 from [run 39792045335](https://github.com/ZJY-HSBL/HashFrontier/actions/runs/39792045335), pinned upstream commit `5064a381fd3b7a540594fadd6f7595fdcc748a6c`.
- Portable `scripts/audit_zombie_native.py` audits author-native phase completeness, declared workload operation counts, and missing cycles. Four regression tests include early, otherwise-successful process termination.
- Audit CSV/JSON, raw converted native-cycle data, per-phase descriptive summaries, provenance with hashes, and two SVG figures are available in `results/v0.4.0-author-v03-audit/` and `figures/`.
- Expanded author-native CI workflow: 85% / 95% occupancy, two *unpaired* independent repeats, 320 requested cycles, pinned q=13 and original variants. RHM/GZHM/GZHM_ADAPTIVE are required to complete; an incomplete TRHM is explicitly recorded, not treated as 320-cycle success. Workflow artifact and job summary retain the results, even on failure.
- No high-tier paper is marked fully reproduced. No proposed new hashing algorithm wins any claim in v0.4.0.

## Methodological limits

The four variants' original OpenSSL-random key traces differ across processes. Their median author-clock ticks/operation are **descriptive only**; not a paired performance claim and not directly comparable to HashFrontier local C++ probes. The scale (2^13 slots) is smaller than the authors' large-workload setting (2^27), and only selected ordered variants are studied. Native throughput values are source clock ticks, not certified nanoseconds across machines.

## Next scientific step

First inspect the new multiple-load, multiple-repeat author-native CI run for stability of TRHM premature termination. Then build a deterministic trace adapter to all four official implementations (keeping algorithm semantics intact) so comparisons are paired; only after correctness and paper-fidelity gates pass should an original bounded-tombstone-redistribution mechanism be evaluated.
