# SIGMOD 2025 Zombie Hashing — provenance and reproduction protocol

**Paper:** Yuvaraj Chesetti, Benwei Shi, Jeff M. Phillips, Prashant Pandey, *Zombie Hashing: Reanimating Tombstones in Graveyard*, Proc. ACM Manag. Data 3(3), 2025, article 236. DOI: <https://doi.org/10.1145/3725424>.

**Author source:** <https://github.com/saltsystemslab/ZombieHT> · BSD-3-Clause. Exact upstream revision audited: `5064a381fd3b7a540594fadd6f7595fdcc748a6c` (master when retrieved 2026-10-22). HashFrontier does **not** vendor or relabel the author source.

The upstream repository has `GZHM`, `GZHM_DELETE`, `GZHM_ADAPTIVE` (ordered compact), and `ABSL_LINEAR_REHASH_CLUSTER_DEAMORTIZED` (unordered/vectorized). The SIGMOD mechanism is **bounded-window pushing of excess tombstones with primitive tombstone positions** under invariant-preserving ordering; it is not identical to periodic global tombstone removal. See the paper's sections 3–4 and its original code's `src/gqf.c` and the modified Abseil submodule. Its `bench/paper_final/churn.sh` uses a `2^27`-slot default (large memory footprint); paper experiments include 75/85/95% occupancy and 5% or 50% updates. The authors' setup requires OpenSSL, submodules and optional architecture-dependent baselines.

## Evidence levels

1. `source_identified`: author implementation, license, commit and build mapping checked — **completed**.
2. `native_smoke_attempted`: GitHub Actions attempts author-source builds and tiny original `hm_churn` runs, with logs uploaded. **This does not establish paper reproduction.** Check the Actions run and its conclusion for this status.
3. `paper_native_reproduced`: original large experiments, author's baselines/figures, hardware caveats, repetitions and independent comparison — **not completed**.
4. `hashfrontier_integrated`: adapt to unified key/operation traces without changing algorithm semantics — **not completed**.

GitHub Action `.github/workflows/zombie-native.yml` pins the original source and attempts ordered `GZHM`/`TRHM` small smoke scenarios at 85% load. These quick runs **are not** author's full settings and do not test the unordered implementation. If dependency retrieval fails, this is a build-infrastructure failure, not an algorithm reproduction result. The workflow does not claim success before Actions is green.

## Why the local global-rebuild controls are separate

`linear_rebuild_2pct`, `linear_rebuild_8pct`, and `linear_rebuild_half_slack` are **HashFrontier classical tombstone-cleanup controls**. They construct fresh arrays of equal capacity, which creates O(n) update outliers and temporary extra memory. None has the primitive or pushing tombstones or deamortized guarantees of ZombieHT. Their purpose is to quantify the cost of stop-the-world maintenance and motivate correct comparison with source-backed ZombieHT.

## Next proof obligations

- Establish clean upstream build and a reproducible original 95%-load churn run on a documented supported CPU, repeating the same code commit.
- Verify the ordered and unordered variants against deletion/insert/query oracle tests, including maximal wraparound and long churn.
- Reproduce per-cycle throughput and latency tables on equivalent resources; retain machine, compiler, flags, core count, seed provenance and raw traces.
- Integrate supported algorithms into matched-operation comparison, separating CPU scalar from SIMD/AVX and GPU tracks.


## v0.3.0: 320-cycle scaled author-native CI

The `zombie-native.yml` workflow now fetches and pins the same upstream revision, builds **four official author variants** (`RHM`, `TRHM`, `GZHM`, `GZHM_ADAPTIVE`), and attempts **320 cycles** at 95% load, q=13 (8,192 slots), 389 deletions followed by 389 insertions and 778 lookups per cycle. It records raw author-generated files, source commit, compiler, hardware metadata, and checksums as a downloadable GitHub Actions artifact. This is explicitly **a scaled reproducibility step, not the full native paper experiment**: the author defaults include q=27, and their OpenSSL-random traces across processes are not paired or seed-stable. Successful GitHub CI confirms execution only; publication-grade figure replication and controlled multi-seed independent performance comparisons remain open.

The six local HashFrontier controls and two new bounded tombstone-repair variants remain in a **separate** matched-trace C++ microbenchmark; their performance cannot be attributed to ZombieHT. A failed local hypothesis is kept in the evidence, not silently dropped.

## v0.4.0 correction: author-native exit code was a false positive

A source-backed post-hoc audit of [the v0.3.0 author-native artifact](https://github.com/ZJY-HSBL/HashFrontier/actions/runs/39792045335) found 960 phase records (320 complete DELETE/INSERT/LOOKUP cycles) for `RHM`, `GZHM`, `GZHM_ADAPTIVE`, **but only 583 phase records for `TRHM`** (194 complete cycles plus cycle 194 DELETE). The GitHub job had exited successfully. In `bench/hm_churn.cc`, `profile_ops()` returns `-1` on `QF_NO_SPACE`, `run_churn()` breaks, and `main()` returns 0. This establishes a misleading experiment-completion signal, although it does not independently prove which exact key triggered this particular stop; a verbose rerun is needed.

- Audit tooling: `scripts/audit_zombie_native.py`, verified with synthetic incomplete-log regression tests.
- Immutable provenance: `results/v0.4.0-author-v03-audit/provenance.json` with source commit, run, artifact ID, ZIP digest.
- Parsed evidence: `audit.csv`, `audit.json`, `native-cycles.csv`, `native-summary.csv`; **TRHM must never contribute to the final-64-cycle comparative cohort** because it has none.
- Timing caution: upstream churn logs use `high_resolution_clock`'s raw `.count()`, so the normalized unit is *native clock ticks per operation*, not guaranteed nanoseconds across toolchains. The independent OpenSSL RNG in each process also prevents paired causal comparisons.
- The v0.4.0 GitHub Action now runs the same original source at q=13 / 85% and 95% load with two repeated independent runs per variant and machine-verifies completed phases. `RHM`, `GZHM` and `GZHM_ADAPTIVE` are required to complete, with failure recorded if not; TRHM may be incomplete but is explicitly labeled as such and the raw evidence is retained.

A successful q=13 scaled test is **not a reproduction of the paper's q=27 large configuration** and does not imply identical original-paper statistics. Full-paper and cross-implementation paired replication remain open.
