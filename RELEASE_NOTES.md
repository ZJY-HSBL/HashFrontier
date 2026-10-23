# HashFrontier v0.7.2 — first VERIFIED measured cross-paper comparison

This release permanently archives the v0.7.1 **actual** same-trace comparisons between SIGMOD 2025 ZombieHT (GZHM/GZHM_ADAPTIVE), PVLDB 2026 TPHT (Flattened/Chained), and std::unordered_set. All 45 scheduled runs at 70/85/95% requested occupancy passed the set oracle and completion tests. Nine binary operation trace SHA-256 digests match all reported rows. The complete original data, source hashes, operation binaries and logs are attached to this Release; the same CSV/real SVG are committed to the repository.

At 95% requested occupancy, Flattened-TPHT averaged 46.16 ns successful lookup, 42.11 ns unsuccessful lookup, and 47.32 ns insertion. Chained-TPHT averaged 60.71, 56.14 and 71.74 respectively, but reported smaller allocations (87,092 bytes vs 134,785 bytes) within the same author library. Zombie GZHM and GZHM_ADAPTIVE show substantially higher insertion cost in this small 32-cycle pilot; this is not a universal paper conclusion.

**No HF-Score**: this is a 64-bit set-membership *pilot* rather than the frozen 64-bit key/value protocol. Native author memory reporting conventions differ and three paired seeds do not support formal inferential claims. The old v0.7.0 preflight failure and v0.7.1 subsequent fix are retained as provenance.
