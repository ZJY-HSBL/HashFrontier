# HashFrontier v0.15.0 — STOC 2025 Partner Hashing §3.2 partial reproduction

- Added external-retrieval/oracle-backed C++20 finite reproduction of Section 3.2 Algorithms 6–8 (query, packed-bin insert, decouple/delete/reencode), plus regression tests and reversible finite-size handling.
- Preserved distinctions: **Algorithms 4–5 retrieval structure embedded into RAM is not implemented**, nor Section 4 fixed-size fully implicit construction, nor Section 5 dynamic resizing.
- Verified randomized correctness: 95,257 checks, 57,600 state mutations and 5,008 real pair decouplings. Local ASan/UBSan passed.
- GitHub Actions run 40182871464 completed 60/60 independently measured configurations, original CSV, SHA-256, CPU/compiler record, verified plots and audit; 23,983 successful of 24,000 RAM writes; 14,400/14,400 completed delete attempts.
- The deletion implementation uses O(n) snapshot rollback; its timing **cannot validate** the paper's O(1) deletion theorem. Bench inputs precondition balanced bin occupancy and keep external retrieval map, counts and random masks.
- Does not calculate HF-Score. The cloud results are engineering measurements of only the partial module, not a cross-paper performance ranking.

Paper: Bender, Kuszmaul, Zhou, *Optimal Non-Oblivious Open Addressing*, STOC 2025, DOI: 10.1145/3717823.3718215.
