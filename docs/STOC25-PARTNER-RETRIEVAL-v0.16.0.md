# STOC 2025 Partner Hashing Algorithms 4–5: conditional implementation

Paper: Bender, Kuszmaul and Zhou, [Optimal Non-Oblivious Open Addressing](https://arxiv.org/html/2503.13628).

**Status: PARTIAL subroutine, NOT full Partner Hashing.**

Algorithm 4 is represented by `query_retrieval(key)`, which reads a partner-pair-encoded RAM word. Key-to-word assignment remains in an **external unordered_map**. The DMPP06 dynamic succinct retrieval black box cited by the paper is not implemented.

Algorithm 5's essential prestate property is tested by `write_batch`: all existing RAM values and target self-loop choices are examined before any relocation; swaps are committed only after the complete disjoint plan is valid. Overlapping or oversubscribed plans are rejected without modifying the physical array. This is a deliberately restricted finite branch of the algorithm, not a full general dynamic in-RAM retrieval update or an O(1)-time result.

## Verification

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

Tests exercise multiword updates, oracle comparisons, permutation preservation, and the failure path: B+1 simultaneous requests to one partner bin of B free positions must reject atomically.

Prior full-size `§4` fixed table and `§5` dynamic resizing **remain unimplemented**. This is not eligible for unified L3 or HF-Score.
