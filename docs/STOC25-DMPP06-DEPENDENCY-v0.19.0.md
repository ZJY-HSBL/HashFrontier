# STOC 2025 Partner Hashing: DMPP06 dependency reproduction (v0.19.0)

**Scope is deliberately limited and source-grounded.** This iteration implements the following genuine components of Demaine, Meyer auf der Heide, Pagh & Pătraşcu (LATIN 2006), *De Dictionariis Dynamicis Pauco Spatio Utentibus*: 

- **§2.1**: finite-field two-independent affine permutation with inverse and exactly reversible `(bucket, quotient)` splitting. GF(2^16) fixed universe. Irreducible polynomial `x^16+x^12+x^3+x+1`.
- **§3.1**: **first-level filter only**, consisting of a quotient-key dictionary in each bucket, stable per-bucket hashcodes, a free-hashcode list, and explicit overflow rejection. A failed first-level insertion *must* be handed to the next level by a future complete implementation. This code returns `false` instead; it does not silently insert overflow keys elsewhere.

**Not implemented:** DMPP06 §2.2 full small-space/high-concentration quotient hash family (its Theorem 5); §3.2 second-level and third-level dictionaries; §4 complete compact retrieval/perfect hashing, high-probability time/space properties; in-RAM embedding of DMPP06 within the STOC 2025 Partner Hashing construction. We intentionally use `std::unordered_map` within the first-level buckets, so bytes/key cannot be claimed to match DMPP06's succinct bound, nor is this the full DMPP06 retrieval data structure. The paper describes its more sophisticated quotient family beyond our finite-field example.

Original papers:
- https://doi.org/10.1007/11682462_34 (LATIN 2006), full source https://people.csail.mit.edu/mip/papers/dyndict/dyndict.pdf
- https://doi.org/10.1145/3717823.3718215 (STOC 2025), full source https://arxiv.org/html/2503.13628

## Exact finite-test protocol

`HF_DMPP06` is a standalone paper-component benchmark, **not a replacement for the STOC 2025 algorithm**. Parameters: `n ∈ {4096,8192,16384}` keys, 64 or 256 buckets, bucket code capacities corresponding to 0%, 10%, 25% slack, four independent seeds, 72 cases. Each run tests fixed insert attempts, all accepted lookups, miss queries of absent keys, and deletion of all accepted keys with oracle verification. Operation timings are exploratory cloud ns/op; overflow is recorded, never treated as a successful insertion.

Run:

```bash
c++ -std=c++20 -O2 -Iinclude tests/test_dmpp06_first_level.cpp -o /tmp/hf-dmpp06-test
/tmp/hf-dmpp06-test
c++ -std=c++20 -O3 -DNDEBUG -Iinclude src/bench_dmpp06_first_level.cpp -o /tmp/hf-dmpp06-bench
/tmp/hf-dmpp06-bench > /tmp/dmpp06-raw.csv
python3 scripts/report_dmpp06_first_level.py /tmp/dmpp06-raw.csv /tmp/dmpp06-audit
```

All uploaded results must contain the raw CSV, a strict audit, environment, SHA-256, plus CSV/SVG summaries. **This is partial reproduction of an underlying cited paper, not complete STOC 2025 Partner Hashing.**
