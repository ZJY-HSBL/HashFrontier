# HashFrontier v0.10.0 — Fixed-platform admission and paper-fidelity audit

- Registered a **single target CPU environment** for all forthcoming formal L3 cross-paper tests: AMD EPYC 9654, 256 GiB RAM, Ubuntu 24.04, GCC 13.3.0, pinned CPU, NUMA, governor and allocator. **Not physically bound yet**: existing GitHub/local timings remain exploratory.
- Added fail-closed host inspection, strict drift checks against existing HF-Score/1.0 (four scenarios, seven metrics and 30 seeds), and 12 protocol regression tests.
- Paper-to-code audited SODA 2025 Bubble-up §4, §5, Appendix B and Appendix C; documented the unimplemented Appendix B choice/collision path, independence assumption, finite parameter mismatch and no empirical-original-figure suite.
- Necessary-condition audit rejects theorem promotion from the old 4096-slot, d=10/d_core=5 trials. This does not invalidate their genuine finite L1 differential correctness tests.
- Revalidated exact Git blob versions in nonformal GCC 14.2 Debian: 2,447,592 Appendix C oracle checks / 888 rebuilds, ASan and UBSan passed; advanced mechanism had 2,587,159 oracle checks and 18 explicit recorded failures.
- Updated the three-paper reproduction ledger, README, source registry and CI. **No new algorithm. No false L2/L3 promotion.**
