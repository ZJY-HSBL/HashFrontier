# SODA 2025 Advanced Bubble-up §5.2: execution and proof-admissibility ledger

Paper: Kuszmaul and Mitzenmacher, *Efficient d-ary Cuckoo Hashing at High Load Factors by Bubbling Up*, [SODA 2025 DOI](https://doi.org/10.1137/1.9781611978322.133) / [full PDF](https://arxiv.org/pdf/2501.02312) (Section 5.2, PDF pp. 11–12).

## Implemented paper mechanisms

`include/hashfrontier/advanced_bubble_up.hpp` is an **independent mechanism-level §5.2 reimplementation**, not an author release and not a proof of Theorem 5.1. It implements:

1. `d_max = gamma + q*d_core`, `gamma = d mod d_core`; phase advances when occupancy reaches `1 - exp(-d_max + alpha)`; capacity remains fixed.
2. Choice is stored explicitly with each occupied slot. Core iff `choice > d_max - d_core`. Type 1 moves choose uniformly among the `d_core` active core hashes and evict any occupant.
3. Type 2 moves first inspect `h_i(x)` for `max(1, choice-d_core) <= i <= d_max-d_core` and use first free slot; if no free slot, move into the core.
4. Positive queries scan `h_{d_max}, h_{d_max-1}, ...` and early return on a match.
5. A consecutive-core-move cutoff declares insertion failure; the engineering implementation **journals and rolls back failed insertions** to avoid corrupting a reference set. This rollback is a safety accommodation not part of the paper's time bound. We use a finite count instead of the unspecified asymptotic `log^{omega(1)} n` constant.

## Explicit limits: do NOT call this L2

- **Theorem 5.1 conditions:** `n^{-1/4} <= epsilon <= exp(-d_core)` and `d_core` a sufficiently large constant depending on `alpha`, with nonconstructive dependencies via the Bell–Frieze theorem. A typical 8,192-slot, 95%-load experiment has epsilon=0.05 and n^{-1/4} ~= 0.105. Hence **it is outside the theorem's asymptotic regime**. A fixed illustrative `d=8`, `d_core=4`, `alpha=0.5` is not sufficient to claim the theorem's constants.
- SplitMix-derived functions approximate independent hashes, not fully random mathematical functions. Choice bookkeeping costs extra memory. Hash coincidences are possible under independent hashes and are allowed.
- This experiment implements **insertion and query only**. Paper Appendix C proposes tombstones/rebuild to incorporate deletions; until that is implemented and verified, Advanced Bubble-up is **not** allowed in the dynamic HF-Score league.
- The `noncore_probes` implementation metric is a physical lookup count, **not** exactly the paper's proof notion of first-time exposure to an independent random location.
- The 30-seed pilot is *one paper's Section 4 vs 5.2* under the same generated keys, not a cross-paper 30-seed HF-Score or an original-figure reproduction.

## Actual exploratory execution

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
./build/hf_bubble_paper_benchmark 30 8192 > results/v0.9.0-bubble-pilot.csv
python3 scripts/analyze_bubble_pilot.py
```

The included local run uses 8,192 slots, 70/85/95% load, 30 paired seeds per load, **2 methods x 3 loads x 30 seeds = 180 observations**. It tests complete construction and successful/unsuccessful membership; `status=failed` appears if the insertion mechanism rejects placement. The 180 rows and statistics are real local container measurements; **not** paper author large-scale measurements, not CPU-pinned, no asymptotic guarantee, and do not mix time readings with existing GitHub-runner results.

## Next scientific reproduction gates

- L1 mechanism mapping exists; correctness oracle and phase-change exercise passed, **but full specification fidelity still requires hash-family choice and failure bound audit**.
- L2 must reconstruct the original paper's asymptotic admissible regime or clearly explain why a finite experiment cannot validate its stochastic proof; compare predicted positive query behavior and original theorem-relevant diagnostics.
- L3 only after moving Appendix C semantics onto the common CPU dynamic, 64-bit key/value and memory protocol. Static insertion-only comparison may be reported separately *without HF-Score*.
