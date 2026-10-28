# HashFrontier v0.20.0 — DMPP06 §3.2 tile-filter reproduction

Faithfully modelled §3.2's finite multi-filter placement and stable tile identifiers using invertible affine finite permutations. Preserves explicit overflow to an unimplemented third level.

**Actually verified:** 298,141 randomized correctness/invariant checks; ASan/UBSan; strict CSV parser tests; independent GitHub Actions runner [40630812596](https://github.com/ZJY-HSBL/HashFrontier/actions/runs/40630812596) completed 72 real benchmark configurations with source, raw CSV, hashes, compiler/CPU environment, summary and generated SVG.

**Key finite observation:** 256 bins, 8192 attempts, 4 seeds; 16/32/64 base tiles show median overflow 16.4185% / 0.1709% / 0%. Do not combine cloud timings with other hardware or score as HF-Score.

**Not implemented:** DMPP06 §2.2 Theorem 5 full high-probability quotient family, original packed constant-time space representation, §3.2 third-level high-performance dictionary, complete dynamic retrieval, or full STOC25 Partner Hashing.
