# HashFrontier v0.24.0 — DMPP06 §2.2 Transformation 4 finite grouped permutations

- Strictly paper-defined *group-shared uniform random permutation of first-order buckets*, plus exact inverse. Finite 16-bit scaling; no Siegel-family implementation or theorem claims.
- 13,893,153 full-domain and grouping checks pass, ASan/UBSan and five CSV-audit unit tests pass.
- [Independent GitHub Actions cloud run 40845661999](https://github.com/ZJY-HSBL/HashFrontier/actions/runs/40845661999): 300 complete raw records, 240 with b<n, 60 configurations and five paired seeds, raw CSV/SVG/provenance archived.
- AMD EPYC 7763, GCC 13.3.0. n=8192, b=1024, shuffled inputs: over-threshold keys 373→350, median hash latency 1.92206→4.36252 ns/key, metadata 1024→1536 bytes.
- Correct scope: finite transformation mechanism only. DMPP06 Theorem 5, Siegel high-independence row shifts, full DMPP06 space guarantees and STOC 2025 Partner Hashing remain outstanding. No HF-Score.

See docs/STOC25-DMPP06-TRANSFORMATION4-v0.24.0.md.
