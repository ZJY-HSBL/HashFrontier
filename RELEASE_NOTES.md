# HashFrontier v0.12.1 — SIGMOD 2025 Zombie Hashing 320-cycle cloud long-churn

This release runs the paper authors' RHM, TRHM, GZHM, and GZHM_ADAPTIVE implementations at pinned source `5064a381fd3b7a540594fadd6f7595fdcc748a6c`. Table implementation files are SHA-256 verified unchanged; only the author benchmark's input RNG and operation fingerprint output are instrumented.

**Real 320-cycle paired experiment:** 85% and 95% load × 5 random seeds × 4 algorithms = **40 scheduled runs; 35 complete; 5 incomplete**. The 85% cohort finished 20/20. At 95%, RHM, GZHM and GZHM_ADAPTIVE each finished 5/5. TRHM crashed 5/5 with status **139**, after 161, 191, 131, 161, 161 cycles. 3,210 paired operation fingerprints matched; not one failure was silently excluded from the completion audit.

Native timings are explicitly **author clock ticks/op**, not nanoseconds, and the GitHub ubuntu-24.04 host here uses a different physical CPU model than the shorter v0.12.0 pilot. The current pilot remains a key-only, nonformal, 8192-slot experiment. It does **not** establish formal across-paper HF-Score eligibility. A crash exit status alone does not establish root cause.

Release includes the full raw author-native logs, per-run statuses, complete-run timing CSV, summary, input trace integrity, environment, SHA-256 manifest, SVG, and the paper-aligned analysis. [Verified cloud run](https://github.com/ZJY-HSBL/HashFrontier/actions/runs/40176343702).
