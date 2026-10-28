# HashFrontier v0.19.0 — DMPP06 LATIN 2006 first-level dependency reproduction

This release continues the reproduction of STOC 2025 *Optimal Non-Oblivious Open Addressing* by tackling its cited **DMPP06** retrieval dependency at source-section level.

**Implemented:** LATIN 2006 §2.1 finite `GF(2^16)` affine quotient permutation and inverse, and §3.1 first-level per-bucket filter using stable local hashcodes, free-code lists, explicit overflow, retrieval/update/deletion, plus exhaustive finite permutation checks.

**Verified:** 2,763,711 C++ randomized/invariant checks, 65,535 nonzero multiplicative coefficients passing inverse verification, ASan/UBSan locally, and 72/72 independent GitHub Actions cloud observations with 4-seed 18-group summary, genuine CSV/SVG, actual environment log and SHA-256 audit.

**Key limitation:** The full DMPP06 Theorem 5 high-probability quotient construction, multi-level filters and succinct implementation are NOT reproduced. This code uses an explicit standard-library map inside buckets and does not satisfy the space bound. It is NOT a complete DMPP06 retrieval data structure, and it is NOT the full STOC 2025 Partner Hashing. Overflow is marked as failure of this filter and must be delegated to later levels in a full implementation. No HF-Score.

Evidence: `docs/STOC25-DMPP06-DEPENDENCY-v0.19.0.md`, `results/v0.19.0-dmpp06/{raw.csv,summary.csv,audit.json,environment.txt,sha256.txt,tests.log}`, `figures/v0.19.0-dmpp06-{hit,overflow}.svg`.