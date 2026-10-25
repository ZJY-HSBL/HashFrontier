# STOC 2025 — Partner Hashing, Section 3.2 finite submodule

Paper: Bender, Kuszmaul, Zhou, *Optimal Non-Oblivious Open Addressing*, STOC 2025, [DOI 10.1145/3717823.3718215](https://doi.org/10.1145/3717823.3718215), [full preprint](https://arxiv.org/abs/2503.13628).

**Status: PARTIAL — only Section 3.2 Algorithms 6–8, under an external retrieval oracle and explicitly finite failure handling.** This is NOT a complete open-addressed table with zero metadata. Algorithms 4–5, Section 4 and Section 5 remain pending. No author source for a complete table has been verified.

## Exact implementation correspondence

- Algorithm 6 (`contains`): compute the would-be logical slot `FindPartner(x)` (arbitrary offset zero for missing keys), reject empty, then calculate the partner's logical slot and compare its actual key with `x`. The lookup never accepts missing keys merely because their hash bin matches.
- Algorithm 7 (`insert_key`): hash to bin `k`, insert at slot `k*B + n_k`, register offset `n_k` in the external retrieval oracle and increment `n_k`; reject a full bin. This maintains the occupied-prefix invariant.
- Algorithm 8 (`erase_key`): decouple the target and the last key in its logical bin if involved in coupling, remember affected encoded RAM words, move the last live key to the target's former logical address, update offsets and bin count, then re-encode the remembered words.
- Algorithm 1–3 mechanics: partner position, RAM reads, self-loop sampling and pair swaps are implemented again inside the dynamic finite module, with explicit underflow/overflow and partner-exhaustion rejection.

## Why this is NOT Algorithm 4–5

The paper uses a DMPP06 dynamic retrieval data structure physically represented by **the encoded RAM itself**, with a batched placement scheme to avoid cyclic dependencies. This implementation uses `std::unordered_map` instead. Its bin counts and the random word masks are also externally allocated, so the metadata-free claim and constant worst-case space/time bounds **do not hold**. We have **not** implemented the paper's batch-simulation Algorithm 5.

Deletion uses an O(n) copy of the physical slots and retrieval map before mutation to permit rollback if finite-size re-encoding fails. This is **an engineering safety mechanism**, not the paper's constant-time deletion algorithm. Accordingly, `delete_ns_attempt` is not a valid measure of the theorem's deletion complexity.

## Correctness tests

`tests/test_partner_dictionary_section3.cpp` tests bins 4/8/16/32, bin sizes 8/16/32, six seeds and 800 randomized operations in each configuration. It checks insertion, deletion, positive and negative membership, encoded word restoration, full physical permutation, bin-prefix occupancy, and rejected operations against independent in-memory oracles. The finite test deliberately prepares balanced bins to keep all index slots occupied and **does not establish** the paper's concentration bound on fully random input.

Run locally:

```bash
c++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Iinclude \
  tests/test_partner_dictionary_section3.cpp -o /tmp/partner_dict_test
/tmp/partner_dict_test
c++ -std=c++20 -O3 -DNDEBUG -Iinclude \
  src/bench_partner_dictionary_section3.cpp -o /tmp/partner_dict_bench
/tmp/partner_dict_bench 5 > /tmp/partner-dict-raw.csv
python3 scripts/report_partner_dictionary_section3.py \
  --raw /tmp/partner-dict-raw.csv --out /tmp/partner-report --figures /tmp/partner-figures --seeds 5
```

The GitHub Actions workflow `.github/workflows/partner-section3-2-cloud.yml` performs its own clean compile and actual cloud run, saves the original CSV, validation audit, source/data SHA and SVGs, and keeps all failures visible.

**Strict interpretation:** The metrics are useful only to verify this source's finite behavior and overhead. They must not be ranked against full Zombie Hashing / TPHT or used in HF-Score.
