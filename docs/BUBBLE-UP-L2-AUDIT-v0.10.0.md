# SODA 2025 Bubble-up — independent L2 fidelity audit (v0.10.0)

**Source:** Kuszmaul & Mitzenmacher, [SODA 2025 DOI 10.1137/1.9781611978322.133](https://doi.org/10.1137/1.9781611978322.133), [29-page manuscript](https://arxiv.org/pdf/2501.02312), §§4–5 and appendices B/C. This is a **theoretical algorithms paper**; it does not supply an empirical benchmark suite whose figure values can be reproduced.

| Paper mechanism | Current project evidence | Fidelity status |
|---|---|---|
| §4 Type 1–4 insertion/eviction | `basic_bubble_up.hpp`; randomized tests | L1 finite, not full paper guarantee |
| §5.2 phases, random core displacement, reverse query | `advanced_bubble_up.hpp` | L1 finite; phase and placement mapping still requires independent line review |
| Appendix B `choice'(x,j)` and coincident hashes | Stores `Slot::choice` directly; SplitMix-derived positions | **Not reproduced**. Separate metadata alters memory and the appendix's independence analysis is not implemented |
| Appendix C tombstones and key reactivation | `advanced_bubble_appendix_c.hpp` | L1 correctness tests pass; tombstones participate in eviction |
| Appendix C augmented-load rebuild and constants | `load + 0.6*(1-load)` experimental threshold, <=4 rehash attempts | **Not paper-level constants**; must resolve epsilon-prime and rebuild semantics before L2 |
| Theorem 5.1 / Corollary 3.1 | Numeric-check script | Current pilot cannot satisfy necessary bounds and does not use fully independent random functions |

The corollary requires `epsilon >= n^(-1/4)`, `epsilon <= exp(-d_core)`, and the main construction prescribes `d=ceil(ln(1/epsilon)+alpha)`. For the current pilot `n=4096, d=10, d_core=5, alpha=.5`, all three live-load cases 70/85/95% violate necessary parts of these conditions. At 95%, `n^(-1/4)=.125 > epsilon=.05 > exp(-5)=.00674`; the theorem's specified d is 4, not implemented 10.

The Appendix C manuscript's final epsilon-prime substitution is not to be silently interpreted as a performance-guarantee-compliant implementation: its printed exponent and adjacent algebra need clarification. What is certain is that physical augmented occupancy must exceed live occupancy. Document any resolution before changing code. The theorem audit only checks **necessary** numeric conditions; even a passing row never proves asymptotic claims, random hash independence, or paper-faithful implementation.

**Revalidated local correctness (non-formal GCC 14.2 / Debian):** v0.9.1 exact upstream Git blobs matched: `advanced_bubble_up.hpp` SHA `71ef92c96014e5e67fd3f4b6fed4cae2313012ae`, Appendix C SHA `297c34abac96945d123f48668c4555931394e96c`, tests SHA `5e67d562621c808e7e526b16bf03576b12f199a3`. 2,447,592 oracle checks, 888 rebuilt tables, 1,410 tombstone-bearing cycles; ASan and UBSan pass. This is **L1 evidence only**. §5.2 separate checks: 2,587,159 oracle checks, 18 explicitly reported insertion failures, which must not be hidden.

**Next author-faithful tasks:** independent §5.2 source-to-paper review; Appendix B choice semantics and duplicate-position handling; parameterized Appendix C rebuild faithful to fully specified paper assumptions; author-native/theoretical workload validation. No performance ranking or L2/L3 promotion at this stage.
