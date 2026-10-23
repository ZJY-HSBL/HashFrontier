# SODA 2025: Basic Bubble-up Cuckoo — Section 4 reproduction

**Paper:** W. Kuszmaul and M. Mitzenmacher, *Efficient d-ary Cuckoo Hashing at High Load Factors by Bubbling Up*, SODA 2025, DOI [10.1137/1.9781611978322.133](https://doi.org/10.1137/1.9781611978322.133); [open manuscript](https://arxiv.org/pdf/2501.02312).

## Exact scope and exclusions

This independent C++20 implementation is based on the **warmup/basic insertion policy in §4**, NOT the advanced scheme in §5. It uses `d = ceil(3 ln(1/epsilon)) + 1`, with epsilon determined by target load (the §4 theoretical construction); the advanced algorithm instead uses `ceil(ln(1/epsilon)+alpha)` and has additional phases and a d-core construction. **We make no claim to reproduce the paper's strongest theorem or its asymptotic probabilistic guarantees.**

**Move-by-move §4 mapping** (`include/hashfrontier/basic_bubble_up.hpp`):

1. Existing key at choice `d`: evict into its `d-1` location (type 1).
2. Existing key at choice `d-1`: evict into its `d` location (type 2).
3. Otherwise, inspect `h_(choice+1)` through `h_(d-2)`; use the first free location (type 3).
4. If none are free, place into `h_(d-1)` and continue the eviction chain (type 4).

The new key begins at choice 0. The choice is stored alongside each occupied slot; the chosen hash slot is always the key's current location. A configurable upper bound on consecutive core evictions triggers an **explicit failure**, not a silent placement error. `contains` inspects the possible locations, descending from `d` to 1; §4 itself does not prove the paper's advanced O(1) expected positive-query guarantee.

## Deliberate engineering deviations

- Instead of ideal mutually independent truly random hash functions, `h_i` uses 64-bit keyed SplitMix-like mixing. Duplicate positions within one key are avoided by deterministic rejection. This is a **finite engineering approximation** and does **not** carry the theorem's random-function assumptions.
- The original §4 analysis concerns **insertions and queries**. We add **direct-slot erase** solely to make the same benchmark trace executable alongside dynamic tables; it is an **independent extension** not a reproduction of the paper's Appendix C tombstone/deletion theorem.
- `uint64_t` keys and an occupied-choice-byte representation are used. This is not an author-supplied implementation. Extra per-slot metadata is counted in the reported allocation.
- `d` adapts to the trace's requested maximum load. This is declared before running a trace and never changes during the run.
- If a run fails to place an element, its status is failed/unsupported in `completeness.csv`; do **not** drop it and compare survivors only.

## Research status

The build includes deterministic correctness/property tests across four loads and twelve seeds, plus an adversarially timed binary-trace driver shared with pinned author-source SIGMOD 2025 ZombieHT and VLDB 2026 TPHT. This is a **Section 4 algorithmic reimplementation and exploratory three-paper comparison**, not a fully validated replication of the paper. The paper's advanced Section 5 implementation remains **pending**.

## Running and verification

The GitHub workflow `.github/workflows/cross-paper-v08.yml` compiles the paper-source ZombieHT and TPHT, runs this Section 4 implementation on **the exact same `.bin` trace files**, and records correctness-gated rows for every algorithm, load and seed. Includes build, positive/negative lookup, insert, delete, update P99 timings. TPHT and ZombieHT memory APIs report incompatible allocation concepts and `std::unordered_set` has no allocator-hook record, so **HF-Score remains forbidden**.

Python report check: `python3 scripts/cross_paper_report.py --observations cross-output/observations.csv --completeness cross-output/completeness.csv --out cross-output`.
