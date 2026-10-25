# HashFrontier v0.17.0 — STOC25 Algorithm 5 finite transactional partner reuse

- Reproduces prestate reads, two-pass decouple/recouple and atomic reject on oversubscribed partner bins. Preserves the old restricted planner for audit.
- Verified 3,600 full-bin cross-swap updates and correctness/sanitizer checks.
- Real [GitHub cloud run 40199001986](https://github.com/ZJY-HSBL/HashFrontier/actions/runs/40199001986): all 54/54 batches of 1,500 attempts, 3 seeds per configuration; actual CSV and SVG archived with SHA256 and environment.
- Cloud runner: AMD EPYC 9V45, Ubuntu 24.04, GCC 13.3.0. Across new planner configurations, 40,500/40,500 actual mutating batches committed; the disjoint predecessor commits zero actual mutating batches. Old versus new timings are NOT equivalent-work comparison.
- Limitations: DMPP06 dynamic succinct retrieval, metadata-free Section 4 and resizing Section 5 are not implemented. This release is a tested paper-subroutine reproduction, not full Partner Hashing or formal L3 / HF-Score.
