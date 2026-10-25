# STOC 2025 — Optimal Non-Oblivious Open Addressing

**Bender, Kuszmaul, Zhou**, STOC 2025, DOI: 10.1145/3717823.3718215; full preprint https://arxiv.org/html/2503.13628.

## Honest implementation status

**INCOMPLETE — Section 3.1 partner-swapping RAM primitive implemented and tested. This is NOT the complete Partner Hashing table.**

The paper's central promise (no external metadata, load factor 1, O(1) updates and lookups, dynamic resizing) depends on the recursive retrieval encoding, dense/sparse RAM, backyard, and Section 5 incremental rebuilding. None of these claims are obtained from the standalone primitive here. This paper is theoretical; the paper does not report implementation benchmark figures for the completed table. No verified original-author source release was found as of this review.

`include/hashfrontier/partner_ram_section3.hpp` reproduces the concrete *encoding rule* and operations in §3.1:

- A full logical array of distinct keys organized into `m` bins of `B` slots. Index bins and partner bins occupy the first/second half. The first half of each index bin denotes one encoded RAM word per slot.
- Paper Algorithm 1: `FindPartner(x)` obtains the logical address of the key. To recover its current physical position, take the logical address of the key physically at its logical address, as in the proof for queries.
- Algorithm 2: `ReadRAM(i)` inspects the key at index slot `i`. If its logical bin is a partner bin, decode bin offset XOR random mask `r_i`; if it is an index-bin self-loop, the word is empty.
- Algorithm 3: `WriteRAM(i,v)` first decouples the previous partner and then samples a self-loop from partner bin `(v XOR r_i)`; the two keys are swapped. Explicit sampling failure is recorded, with restoration of the previous value.

### Differences versus the final theorem

1. The key-to-logical-slot retrieval function is an **external `std::unordered_map` oracle**. In the real completed algorithm, the retrieval structure itself has to be stored in permutation-encoded RAM. Here it is intentionally external to isolate the recursion-free primitive.
2. Random masks `r_i` are externally allocated; hashing/bin assignment is prespecified at initialization. Constructing the full table from arbitrary key sets, the Section 3 prefix-of-bin invariant under insertions/deletions, and the Section 4 backyard are NOT implemented.
3. Tests may explicitly refuse writes if no partner self-loop exists. The paper's occupancy assumptions and high-probability argument require much larger asymptotic bin sizes; no claim of guaranteed constant time or no failure is made.
4. The reported `physical_bytes` **excludes** external retrieval oracle and masks. `external_metadata_lower_bound_bytes` is a lower bound only; allocator capacity/overhead is additional.
5. This standalone primitive has **no dictionary INSERT/DELETE**, and must NOT enter cross-paper HF-Score or be ranked against ZombieHT/TPHT.

## Reproducing the actual stage experiment

```bash
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Iinclude tests/test_partner_section3.cpp -o /tmp/partner_test
/tmp/partner_test
g++ -std=c++20 -O3 -DNDEBUG -Iinclude src/bench_partner_section3.cpp -o /tmp/partner_bench
/tmp/partner_bench 5 > results/stoc25-partner-section3/raw.csv
python3 scripts/report_partner_section3.py --raw results/stoc25-partner-section3/raw.csv --out results/stoc25-partner-section3 --seeds 5
```

Cloud workflow `.github/workflows/partner-section3-cloud.yml` runs these commands on GitHub Actions Ubuntu 24.04; archived output includes raw CSV, strict completeness audit, environment fingerprint, hashes and charts. Measured `ns/op` values include finite engineering overhead and **are not values predicted or measured by the authors**.

## Next algorithms not yet replicated

Implement and validate §3 retrieval data structure encoded into RAM (avoiding cyclic dependencies in Algorithms 4–5), complete §3 key Insert/Delete/Query, §4 strong fixed-size construction (frontyard/backyard, dense/sparse RAM) and §5 resizing. Every module requires its own tests before the paper can be labeled fully reproduced.
