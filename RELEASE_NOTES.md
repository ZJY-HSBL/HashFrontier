# HashFrontier v0.11.0 — SODA 2025 Appendix B executable algorithm reproduction

- Adds **paper Appendix B Equation (10)** `choice′(x,j)` reconstruction on actual eviction, plus the stated early redirect when a non-core location coincides with a core hash location. These are *paper mechanisms*, not new algorithm inventions.
- Preserves the §5.2 original stored-choice mode as an explicit comparison option, without changing the default or breaking previous API usage.
- Propagates the Appendix B option through Appendix C transactional tombstone-clearing rebuilds. Existing Appendix C tests remain passing.
- New correctness tests: **269,540 checks, 40 true phase-progressions, 101 collision redirects, 570,235 choice′ evaluations**, ASan/UBSan clean in the local cloud container. Existing Appendix C regression: **2,447,592 checks, 888 rebuilds**.
- **120 / 120 real pilot observations** (2 implementations × 2 capacities × 3 loads × 10 seeds) checked in CSV, with reproducible SVG visualizations and provenance.
- Fixes an *experimental coverage gap*: prior `d=8,d_core=4,alpha=.5` pilot never crossed its 96.98% phase threshold; this pilot deliberately crosses at ~87.13%. Finite `d_core=3` **does not** satisfy Theorem 5.1 and no theorem/optimality claims are made.
- New independent GitHub Actions workflow runs the Appendix B regression and repeats all 120 benchmark rows, uploading results for independent verification.
- All measurements remain **cloud pilot / correctness stage**. No claim of physical-host L3 comparison, no HF-Score, no original innovations.
