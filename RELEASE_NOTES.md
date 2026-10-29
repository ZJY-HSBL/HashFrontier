# HashFrontier v0.22.0 — DMPP06 Section 2.2 finite reversible column quotient hash

- Implemented §2.2 Transformation 2 (per-row circular shifts) and Transformation 3 (independently seeded bijective per-column permutation, finite modular-affine substitute).
- Verified 12,583,011 full-domain permutation and inverse checks; AddressSanitizer/UBSan pass.
- Independent GitHub Actions run 40838246871: 160/160 measured configurations, 16 summaries, raw CSV/SVG/environment/source SHA-256 all preserved; AMD EPYC 7763 cloud VM, Ubuntu 24.04, GCC 13.3.0.
- Ten-seed median collision-participating keys for 1,024 contiguous keys: 530.5 / 255 / 134.5 at 1,024 / 2,048 / 4,096 buckets.
- **NOT** the paper's Siegel high-independence row-shift hash family or its field-based 2-universal per-column permutation. Theorem 5, succinct representation, cited 1990 high-performance dictionary, full DMPP06 and STOC 2025 Partner Hashing remain unimplemented. No HF-Score.

See docs/STOC25-DMPP06-COLUMN-v0.22.0.md for exact reproduction boundary.
