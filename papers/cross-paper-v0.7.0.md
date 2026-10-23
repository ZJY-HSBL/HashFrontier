# v0.7.0: first native cross-paper comparison / 首次作者源码跨论文对照

**Papers**

1. SIGMOD 2025 — *Zombie Hashing: Reanimating Tombstones in Graveyard*, DOI `10.1145/3725424`. Native author's `saltsystemslab/ZombieHT` pinned to `5064a381fd3b7a540594fadd6f7595fdcc748a6c`; build `GZHM` and `GZHM_ADAPTIVE` variants **without modifying author algorithm code**.
2. PVLDB 19(9), 2026 — *Succinct and Fast Tiny Pointer Hash Tables*, DOI `10.14778/3819518.3819542`. Native author's `Xilinion/TPHT` pinned to `3c39de26faba11bc1a037a1dfd24e3c633ac8987`; exercise both `flatten_tpht64` and `chained_tpht64` sequential interfaces **without modifying author algorithm code**.
3. `std::unordered_set<uint64_t>` standard-library engineering control; not a paper reproduction and not the frozen Robin Hood HF-Score reference.

## Fair common operation subset

All candidates receive the same **serialized binary file**, with identical randomized 64-bit keys, build order, delete stream, insert stream, successful and unsuccessful queries. This is the **precise set-membership track**. TPHT uses a 1-byte constant value with `put/get/remove`, ZombieHT uses its original 0-bit-value `g_insert/g_lookup/g_remove`; every returned membership result is cross-checked against a standard `std::unordered_set` oracle. The input files are generated *independently of the algorithms* and are SHA-256 checked. Original algorithm sources and SHA digests are preserved.

TPHT fixed mode is permitted to expand internally in a structural-overflow case: its actual ending capacity is reported, and such a result must not be misinterpreted as running at identical physical occupancy. ZombieHT is built with `QF_HASH_NONE`, `QF_KEY_IS_HASH`; inputs are already mixed 64-bit keys per the author's intended interface. TPHT uses its own documented hash implementation and explicit nonzero hash seed (only inputs are shared, not internal hash functions).

**Pilot matrix**: 8192 requested slots, 70/85/95% initial occupancy, 3 seeds 101/102/103, 32 cycles replacing 5% of keys per cycle, 128 positive and 128 negative queries per cycle, 5 algorithms = **45 scheduled independent runs**. Before the matrix, a 2048-slot, 4-cycle 75% occupancy preflight requires all five variants to pass the oracle and matching-trace test. Every failed/timeout run is retained in `completeness.csv` and raw logs, not excluded invisibly. The Action attaches complete binary input files so another researcher can replay exact same operations.

**Measurements**: wall-clock nanoseconds per initial build, hit, miss, individual insertion, individual deletion, and raw nearest-rank update-operation P99. The per-operation nanosecond measurements include the two clock calls and are **exploratory**, especially short operations. `author_reported_memory_bytes` is *not* a normalized allocator-traced footprint: ZombieHT adds an assumed value compensation to its metadata size, TPHT reports native allocations, the STL reference has no comparable figure. Hence **HF-Score v1.0 must NOT be computed from these memory numbers**, and this set-membership track does not satisfy the frozen 64-bit key+value contract. Formal result = pending.

Commands and source pinning are recorded in [the workflow](../.github/workflows/cross-paper-pilot.yml). The workflow also writes `observations.csv`, `completeness.csv`, `summary.csv`, `comparison.svg`, environment metadata, per-run logs, author source hashes and trace hashes. The final released dataset and its completion rate, **not merely an exit-zero GitHub Actions state**, determine what comparisons can be stated.

## Next evidence gates

1. Check actual CI pilot success/failure matrix, inspect crashes, failed insertions and data corruption.
2. Extend the same exact trace format to `robin_hood` as an actual key-value adapter, and include Bubble-up Cuckoo only after faithfully implementing the SODA 2025 algorithm (never substitute ordinary cuckoo).
3. Add allocator instrumentation or RSS-isolated measurement for comparable physical memory; use a genuine `uint64 -> uint64` dictionary track and freeze hardware/30 seeds before running full HF-Score.
4. Compare author paper native experiment configurations independently from this cross-paper pilot.
