# SODA 2025 Advanced Bubble-up — Appendix B & C executable reproduction (v0.11.0)

**Paper:** Kuszmaul, Mitzenmacher, *Efficient d-ary Cuckoo Hashing at High Load Factors by Bubbling Up*, SODA 2025, DOI 10.1137/1.9781611978322.133, [arXiv:2501.02312](https://arxiv.org/pdf/2501.02312).

## Scope / 准确复现边界

- `include/hashfrontier/advanced_bubble_up.hpp`: §5.2 core eviction, non-core search and phase progression (idealized `choice(x)` engineering version).
- **New, separately selected:** Appendix B `choice′(x,j) = max{i : h_i(x)=j}` evaluated on displacement (Equation 10), plus Appendix B p.26 **early core redirect** before examining a non-core location if it aliases any current core hash location. This is the paper's mechanism, not a novel optimization.
- `include/hashfrontier/advanced_bubble_appendix_c.hpp`: Appendix C tombstones participating in normal eviction, reactivation and periodic full rebuild. The wrapper now accepts `appendix_b_choice_prime=true` and **preserves that mode after each rebuild**.
- The finite implementation uses independently seeded SplitMix64-derived *deterministic* hash functions, not the paper's ideal full-random functions. The paper assumes `d_core` large enough as a function of α and `ε <= exp(-d_core)` with `n^(-1/4) <= ε`; **our practical n=4096/16384, d_core=3, alpha=0.95, and 5–30% gaps DO NOT meet that theorem**.
- For implementation auditing, per-slot `choice` metadata is **still allocated** in both variants, but the Appendix B variant uses computed `choice′` on displacement, not stored `choice`, to decide the next move. This **does not** establish the paper's memory-optimal representation or theoretical bound. Reported per-key bytes include this metadata.
- These are set-membership experiments (keys, no values). The algorithms must **not** enter a 64-bit key–value dictionary HF-Score without a semantics-preserving adapter; this pilot does not produce HF-Score.

## Defect in the preceding experimental coverage

The previous `d=8,d_core=4,alpha=.5` small-table setting begins with `dmax=4`, and its next-phase threshold is `1-exp(-3.5)≈96.98%`, above the 95% maximum. Therefore previous 70/85/95 measurements **never exercised an Advanced §5.2 phase transition**. This version deliberately uses `d=9,d_core=3,alpha=.95` so phase one ends at `1-exp(-2.05)≈87.13%`, and 95% tests cross that boundary; these finite parameters are **NOT** theorem-admissible. Regression tests explicitly fail if phase transitions or Appendix B collision redirects do not occur.

## Commands / 如何复现

```bash
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Iinclude tests/test_appendix_b.cpp -o /tmp/hf_b_test
/tmp/hf_b_test
g++ -std=c++20 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude tests/test_appendix_b.cpp -o /tmp/hf_b_san
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 /tmp/hf_b_san
g++ -std=c++20 -O3 -DNDEBUG -Wall -Wextra -Wpedantic -Iinclude src/appendix_b_benchmark.cpp -o /tmp/hf_b_bench
/tmp/hf_b_bench 10 > results/v0.11.0-appendix-b-cloud-pilot.csv
python3 scripts/plot_appendix_b.py
```

GitHub Actions `.github/workflows/bubble-appendix-b.yml` executes these **again** on an independent runner and uploads its own CSV, SVG and complete compiler/environment record. The checked-in CSV is from the separately described ChatGPT ephemeral cloud container; do not silently substitute runner timing numbers.

## Real finite cloud container observations

- AMD EPYC 9V74 virtual CPU, Debian 13, GCC 14.2.0, effective cgroup RAM cap 4 GiB. **Not** the planned EPYC 9654 formal machine.
- 2 capacities (4096, 16384), 3 loads (70, 85, 95%), 10 common seeds (101–110), 2 implementations = **120 real runs, 120 correctness passes, 120 complete insert sequences**.
- Tests: **269,540 differential/queried checks**, **40 tests entering phase 2**, **101 non-core/core collision redirects**, **570,235 `choice′` computations**, as printed by the actual executable. Historical Appendix C test: **2,447,592 differential checks**, **888 rebuilds**; these numbers are from that *separate* regression test, not the benchmark.
- `results/v0.11.0-appendix-b-cloud-pilot.csv` contains every observation including the status and relocation statistics. `results/v0.11.0-appendix-b-summary.csv` is a deterministic summary of the raw data. `results/v0.11.0-appendix-b-provenance.json` records the exact runtime/compiler and SHA-256 source hashes. Figures are generated from raw CSV, never invented.

### Important interpretations

The calculated `choice′` introduces recomputation overhead; do not infer that it dominates or establishes asymptotic complexity from these small, virtualized CPU measurements. Stored-choice and Appendix B variants have different computational semantics around duplicate hash locations and may use different placements. Build timing is a single batch per seed, not a low-noise microarchitectural study. The source and raw measurements are the deliverables; final on-premise comparison is deferred as requested.

## Pending faithful reproduction items

- Do not claim true-random hash independence or the theorem's optimal-`d` guarantees from a SplitMix64 finite implementation.
- Verify semantics of the Appendix B displacement and corrupt-element analysis under alternative hash families; trace and report non-core collision redirects.
- Larger-scale tests and separate Appendix C dynamic data on actual cloud runners. No need to wait for final fixed hardware to keep developing and testing.
