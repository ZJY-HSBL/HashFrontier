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

## v0.3.0 scaled author-native 320-cycle gate

The separate GitHub Actions job pins the original author revision and attempts RHM, TRHM, GZHM and GZHM_ADAPTIVE at q=13, 95% load, 320 cycles (389 deletes, 389 inserts, 778 queries each). Author output, commit, CPU and compiler are archived. This is **not** a full q=27 paper reproduction. Original OpenSSL-random inputs are not fixed-seed paired across variants. The two new bounded-repair controls in HashFrontier are explicitly **not ZombieHT**.
