# LATIN 2006 DMPP06 §3.2 — finite second-level multi-filter reproduction

**Upstream algorithm:** Erik Demaine, Friedhelm Meyer auf der Heide, Rasmus Pagh, Mihai Pătraşcu, *De Dictionariis Dynamicis Pauco Spatio Utentibus*, LATIN 2006. [Full manuscript](https://people.csail.mit.edu/mip/papers/dyndict/dyndict.pdf), §3.2.

## Reproduced mechanism

For each outer quotient bucket, a sequence of invertible quotient permutations is used. Filter level `i` has `first_tiles / 2^i` tiles. Each occupied tile stores only the **offset within that tile** (plus a separate experimental payload). During insertion, the first level with an empty tile wins. Query recomputes the unique tile candidate on each level and verifies the stored offset. An occupied tile's identifier is its stable perfect-hash code; deletion clears the tile and does not renumber survivors. When all filters reject an element, it is marked **overflow to the still-unimplemented third level**, not falsely reported as inserted.

Original manuscript describes random quotient permutations, geometric tile counts, in-tile offsets, and the third-level handoff. This finite code uses per-bucket independently seeded odd affine permutations modulo a power-of-two quotient domain; they are **bijective but not a substitute for the manuscript's Theorem 5 concentration/independence assumptions**.

## Incomplete theory and research limits

- In contrast to the paper's word-packed compact implementation, this uses `std::vector<Tile>` and a separate payload, so memory efficiency and `O(1)` worst-case theorem are **not reproduced**.
- The finite setting uses 16-bit keys and 64/256 quotient buckets, 4 geometric levels, base tile counts 16/32/64. The actual paper's number of filters depends asymptotically on `n`; these are experimental parameters and do not validate the theorem.
- The §2.2 high-probability quotient-hash family and §3.2 third-level high-performance dictionary remain unimplemented.
- This is a separately verified DMPP06 dependency component; it is NOT the STOC 2025 full Partner Hashing algorithm and must not enter HF-Score.

## Cloud experiment

72 samples = bucket bits {6,8} × first tile counts {16,32,64} × attempted insertions {4096,8192,16384} × four seeds, each with 10,000 verified hit and miss queries and approximately 25% deletions of successful entries. Records include insertion overflows and stable-code checks, not just successful timings. The CI worker captures its own actual CPU, compiler, and kernel. Source, raw CSV, summary CSV, environment and SVGs are kept in `results/v0.20.0-dmpp06` and `figures/`.

```bash
c++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Iinclude tests/test_dmpp06_second_level.cpp -o /tmp/dmpp06-test
/tmp/dmpp06-test
c++ -std=c++20 -O3 -DNDEBUG -Wall -Wextra -Wpedantic -Iinclude src/bench_dmpp06_second_level.cpp -o /tmp/dmpp06-bench
/tmp/dmpp06-bench > /tmp/second-level.csv
python3 scripts/report_dmpp06_second_level.py /tmp/second-level.csv /tmp/second-level-report
```
