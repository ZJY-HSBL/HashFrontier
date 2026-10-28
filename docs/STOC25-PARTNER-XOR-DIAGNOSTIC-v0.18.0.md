# STOC 2025 Partner Hashing — encoded XOR retrieval diagnostic (v0.18.0)

Paper: Michael A. Bender, William Kuszmaul, Renfei Zhou, *Optimal Non-Oblivious Open Addressing*, STOC 2025, https://doi.org/10.1145/3717823.3718215.

## Replication status: **NOT the paper's DMPP06 retrieval structure**

The STOC 2025 paper's Section 3.1 and Algorithms 4–5 **use a dynamic succinct retrieval data structure** (Theorem 3.3, DMPP06) with compact offsets and high-probability constant-time updates. Our existing §3 implementation uses an external unordered-map oracle, so the full paper has NOT been reproduced. This release implements an explicitly separate **test harness** to check that arbitrary retrieval *values* can be physically encoded as partner swaps and read back without accessing an external key-index oracle during queries.

**Experimental control only:** A standard 3-choice peeling-based XOR retrieval table encodes values in the already implemented PartnerRAMSection3. Each XOR cell is one partner-encoded RAM word (`nullopt` represents zero); a lookup XORs three physical RAM reads. This is NOT the author's dynamic retrieval algorithm or a reproduction of DMPP06. A separate `std::unordered_map` stores active keys and values, and each insert/delete/update fully reconstructs the XOR layout. Thus update time scales with table size and has no O(1) high-probability guarantee. The physical array also retains the existing separate logical-slot oracle/masks.

This isolated experiment does NOT justify any of the paper's no-metadata, load-factor-1, full dynamic-resizing, or worst-case guarantees. The code is not a competitor and MUST NOT be entered into cross-paper HF-Score.

## Implemented details

- Hash each 64-bit key to three distinct RAM-word indices with keyed 64-bit mixing.
- Build the 3-uniform hypergraph, peel degree-1 vertices, assign XOR words in reverse peeling order; retry seeds on non-peelable cores.
- Stage target RAM word encodings and use the existing `write_batch_reuse` partner-swap commit; verify every resulting word via physical RAM reads.
- Rebuild on every logical insert, update or deletion; retain old physical and logical state on failure; reject occupancy above 20% of RAM words.
- Verify retrieval values of all present keys; **absent-key retrieval returns an arbitrary value and is not a membership query**.
- Report a lower bound on the **EXTERNAL** key registry + original metadata; no fabricated claims about metadata-free memory.

## Execute

```sh
g++ -std=c++20 -O2 -Iinclude tests/test_partner_xor_retrieval.cpp -o /tmp/xor-test
/tmp/xor-test
python3 -m unittest discover -s tests -p test_partner_xor_report.py -v
g++ -std=c++20 -O3 -DNDEBUG -Iinclude src/bench_partner_xor_retrieval.cpp -o /tmp/xor-bench
/tmp/xor-bench 3 > /tmp/xor-raw.csv
python3 scripts/report_partner_xor_v018.py --raw /tmp/xor-raw.csv --out /tmp/xor-report --seeds 3
```

The 18-run cloud matrix is 3 bin counts (64/128/256) x 2 bin sizes (32/64) x 3 seeds, with an actual verified build, 20,000 successful retrieval queries, updates and deletions per run. All timings are exploratory and **not** the performance of the STOC paper's Partner Hashing algorithm.

## Research gate after this diagnostic

The next legitimate fidelity milestone is to implement the **DMPP06 dynamic retrieval black box**, show the relationship between its actual bit storage and the capacity of the encoded RAM, and test Algorithm 4/5 updates against the *actual* dynamic retrieval structure, not against this XOR whole-rebuild control. Section 4's advanced RAM and Section 5's dynamic resizing remain pending.
