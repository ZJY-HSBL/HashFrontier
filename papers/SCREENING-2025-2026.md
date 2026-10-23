# HashFrontier literature screening / 2025–2026 高水平论文筛选

**Screened on:** 2026-10-23. **Scope:** published 2025-01-01 to 2026-10-23; FOCS 2026 accepted-but-not-yet-held entries marked separately. This is a *verified candidate scan*, not proof of an exhaustive literature census. A journal without a validated year/category quartile remains pending; an arXiv-only manuscript is never promoted as a premier-venue publication.

**Scope restriction:** Current work is *reproduce published algorithms and compare on equivalent workloads*. Do not design new algorithms or add an unlabelled engineering optimization. A paper's native task and the cross-paper comparable subset must be reported separately. Standalone theoretical bounds do not automatically imply an implementable software program.

## Inclusion matrix

| ID | Venue/year | Algorithm / paper | Track | Admission | Reproduction evidence / next action |
|---|---|---|---|---|---|
| zombie-2025 | SIGMOD/PACMMOD 2025 | [Zombie Hashing](https://doi.org/10.1145/3725424) | sequential CPU dynamic | **P1 retain** | Author source; native scale and author figures not yet L2; matched 64-bit-value L3 incomplete |
| tpht-2026 | PVLDB 2026 | [Succinct and Fast Tiny Pointer Hash Tables](https://doi.org/10.14778/3819518.3819542) | sequential CPU dynamic | **P1 retain** | Author implementation; original memory/YCSB replication and 64-bit-value standardized L3 incomplete |
| bubble-2025 | SODA 2025 | [Efficient d-ary Cuckoo Hashing at High Load Factors by Bubbling Up](https://doi.org/10.1137/1.9781611978322.133) | online insertion + query | **P1 retain** | §4 Basic and §5.2 Advanced engineering implementations; verify theorem's admissible constants; Appendix C deletions separately |
| stoc25-open | STOC 2025 | [Optimal Non-oblivious Open Addressing](https://acm-stoc.org/stoc2025/toc.html) | theoretical dynamic | **P2 retain** | Full paper specification + exact data structure needed; no generic baseline substitution |
| stoc26-greedy | STOC 2026 | [Greedy Open Addressing Revisited: Beyond Yao’s Lower Bound](https://acm-stoc.org/stoc2026/toc.html) | theoretical probing | **P2 retain** | Check algorithm has executable insertion/search rules; build-only versus full dynamic operation coverage |
| focs26-twochoice | FOCS 2026 **accepted** | [The Power of Two-Choice Linear Probing](https://focs.computer.org/2026/accepted-papers/) | theoretical probing | **P2 retained provisional** | Two-choice variants; official conference occurs November 2026, publication not falsely claimed |
| focs26-bucket | FOCS 2026 **accepted** | [Fast Insertion for Bucketized Cuckoo Hashing](https://focs.computer.org/2026/accepted-papers/) | bucket cuckoo | **P2 retained provisional** | Bucket occupancy, insertion evictions; official conference occurs November 2026 |
| stoc25-static | STOC 2025 | [Optimal Static Dictionary with Worst-Case Constant Query Time](https://doi.org/10.1145/3717823.3718278) | CPU static dictionary | **P3 retain separate** | Proof-heavy, no deletion. Score on build/queries/bits-per-key only |
| sigmod26-meep | SIGMOD 2026 | [Meep Hashing: Ultrafast and Compact Minimal Perfect Hashing for Practical Large-Scale Lookup Systems](https://2026.sigmod.org/sigmod_papers.shtml) | CPU static MPHF | **P3 retain separate** | Needs full implementation/metadata check, MPHF alone does not validate absent-key membership |
| sigmod25-gph | SIGMOD/PACMMOD 2025 | [GPH: An Efficient and Effective Perfect Hashing Scheme for GPU Architectures](https://doi.org/10.1145/3725406) | GPU | **P4 retain separate** | GPU kernel/VRAM/native hardware validation |
| stoc25-concurrent | STOC 2025 | [History-Independent Concurrent Hash Tables](https://doi.org/10.1145/3717823.3718283) | concurrent | **P4 retain separate** | Linearizability/LL-SC/history-independence correctness needed |

## Exclusion / hold matrix

| Candidate | Disposition | Scientific reason |
|---|---|---|
| [Linear Hashing Is Optimal, STOC 2025](https://acm-stoc.org/stoc2025/toc.html) | **Exclude core algorithm replication** | Balls-into-bins **maximum load bound** of F2-linear maps, not a construction/collision-resolution table with insertion/lookup/erase algorithm; potential hash-family background only |
| [Fingerprint Filters Are Optimal, FOCS 2025](https://focs.computer.org/2025/accepted-papers/) | **Exclude core comparison** | An information-theoretic **lower bound** for approximate membership filters; error-bearing filter is not an exact hash-table dictionary |
| [Tight Bounds and Phase Transitions for Incremental and Dynamic Retrieval, SODA 2025](https://epubs.siam.org/doi/book/10.1137/1.9781611978322) | **Exclude exact-dictionary benchmark** | Retrieval structures omit explicit key storage; nonmember semantics are not exact membership |
| [The Power of Two-Choice Linear Probing, arXiv 2610.00688](https://arxiv.org/abs/2610.00688) | **Not excluded** | This is the public version of an officially accepted **FOCS 2026** paper; do not misclassify as arXiv-only or as already conference-published |
| [2024 FOCS open addressing papers](https://focs.computer.org/2024/) | **Exclude date window** | Official 2024 venue, even if arXiv revision happened in 2025/26 |
| SOSD indexing papers or LSH approximate similarity studies | **Exclude core benchmark** | Different API/semantics or approximate similarity instead of exact map construction/collision resolution |

## Native-paper vs unified comparison gates

- **L1 – specification-faithful implementation:** paper-specific pseudocode mapping, invariant test and explicit implementation differences. A small engineering parameter does **not** establish theorem-admissible constants.
- **L2 – original-paper evidence:** full original workloads, scale and baselines; paper figures/numbers checked. A correct small-scale run is *not* L2.
- **L3 – uniform benchmark:** 64-bit key/value semantics (or separate set-only track), same trace SHA, hardware, allocator/memory accounting, 30 paired seeds, fixed score protocol; absence of any mandatory metric excludes formal HF-Score.
- Static MPHF, concurrent and GPU remain **separate**. Never mix throughput across incompatible hardware/operation sets.

## Next execution order (no original-algorithm invention)

1. Advanced Bubble-up §5.2: verify phase transitions, choice invariant, core random walk, finite-size / ideal hash limitations; implement Appendix C native deletion mechanism separately; tackle theorem-admissible parameter range and primary publication experiments.
2. ZombieHT: original q=27, 75/85/95 loads, author variants/figures and paired 64-bit values; diagnose TRHM fail without modifying algorithm to hide it.
3. TPHT: original paper's YCSB and space-efficiency configuration; both flattened/chained; official source commit/CLI manifest.
4. Common CPU workload: key/value 64-bit, exact membership and values, memory via common allocator instrumentation (avoid API-reported bytes mixed with RSS), >=30 paired seeds and same machine; *only then* formally compute HF-Score.
5. Execute P2 theoretical papers sequentially after full pseudocode mapping; do not label approximate/pedagogical variants as formal reproductions.
