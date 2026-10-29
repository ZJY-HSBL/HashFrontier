# HashFrontier v0.25.0 — SIAM J. Computing 2004 Siegel Lemma 2.6 finite graph mechanism

- Exact finite *k-peelable* neighbor graph certification and XOR of independent random right-side table cells (Siegel 2004, Lemma 2.6; cross-checked with Thorup FOCS 2013 §I.B).
- 4,416,204 exact nonempty subset certificates; 130 exhaustive tiny-graph joint distribution checks; 1,683 additional invariants; ASan/UBSan passed.
- [Independent cloud benchmark run 40848770409](https://github.com/ZJY-HSBL/HashFrontier/actions/runs/40848770409): 40/40 raw rows, eight configurations × five seeds; GitHub Actions AMD EPYC 9V74, GCC 13.3.0.
- 64-key examples: degree 3 → 2.22687 ns/eval and minimum 384 B adjacency; degree 4 → 2.56612 ns/eval and minimum 512 B adjacency; each includes a separate 1,024 B random-word table.
- **Not the full Siegel explicit construction**, no sublinear adjacency storage, no large-universe k=n^Ω(1) proof; the benchmark uses pseudorandom test table data. Not full DMPP06 or STOC 2025 Partner Hashing. No HF-Score.

See docs/SIEGEL-2004-LEMMA2.6-FINITE-v0.25.0.md.
