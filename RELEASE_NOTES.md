# HashFrontier v0.13.0 — PVLDB 2026 TPHT original-author cloud reproduction

- Run the **unmodified author source** (Xilinion/TPHT commit 3c39de26faba11bc1a037a1dfd24e3c633ac8987), including the upstream native correctness/SIMD suite.
- **576 measured latency rows**: 70%, 85%, 95% loads with 8-byte values plus 85% with 1-byte values, 32/64-bit keys, 2^10..2^18 keys, Chained and Flattened, insert/hit/miss/remove.
- **640 measured memory rows**: 100000/200000 inserted keys, 20 target load factors, 4 value widths, 32/64-bit keys and both variants.
- All six untouched author CSVs, source checksums, environment capture and test log are preserved; strict cloud audit rejected missing tuples, duplicates, nonpositive timing and partial insertions.
- Real 64-bit key/value results (262144 keys, 85% load): Flattened success lookup 12.14 ns/op and Chained 21.86 ns/op; 200000-key memory Chained 19.92, Flattened 25.07 bytes/key.
- README and dedicated results document contain SVG charts plotted from actual raw CSV.
- Cloud exploration only, **no final HF-Score**. No original algorithm modifications.
- Benchmark run: https://github.com/ZJY-HSBL/HashFrontier/actions/runs/40179300855.
