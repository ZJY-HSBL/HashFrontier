# Reproduction admission protocol / 论文复现准入协议

**Search window:** 2025-01-01 through 2026-10-22 (inclusive). Registry v0.1.0 is an **initial verified shortlist**, *not a systematic, exhaustive review*.

## Venue filter

Admit primary papers only if published in a relevant Nature/Science/Cell family venue, a **year-and-category verified** Q1/Q2 journal, or an established premier CS venue such as STOC, FOCS, SODA, SIGMOD and VLDB. A paper on arXiv alone is **not sufficient**. Venue standing is not a substitute for technical relevance.

## Paper lifecycle

`discovered -> venue_verified -> specification_reviewed -> implemented -> correctness_verified -> reproduction_attempted -> independently_checked`.

Each reproduction needs: paper DOI and version; algorithm pseudocode mapping; implementation or explicitly labeled reimplementation; compilation manifest; correctness and adversarial testing; original authors' reported setting; raw runs and paired seeds; replication deviations; final reproducibility assessment. Never promote a paper above `implemented` on the basis of a classic baseline.

## Fair comparison

Dynamic sequential set tables, concurrent tables, static minimal-perfect hash functions, and GPU tables are **distinct tracks**. Comparison across tracks is allowed only for operations both support, with platform and capability constraints explicitly displayed. The primary sequential track uses identical key sets, hash mixing, load targets, operations and random seeds. Keep raw measurements, never cherry-pick; plot distributions and tail percentiles in later stages. Report CPU/memory/runtime/compiler, confidence intervals, and paired tests before substantive performance claims.

## Baseline status for v0.1.0

`linear_tombstone`, `linear_backshift` and `robin_hood` are **classical, independent baseline implementations**; none is labeled a replication of any 2025 paper. All registry entries are `queued`.

## Next screening actions

Expand proceedings scans across 2025/2026 STOC/FOCS/SODA/SIGMOD/VLDB (and Q1/Q2 verified venues), log inclusion/exclusion and DOI. Start Zombie Hashing with authors' available reference implementation if license and behavior permit; reproduce its own published test conditions before any cross-method comparisons.

## Fixed composite-score method

HF-Score v1.0 is defined by [`scoring/PROTOCOL.md`](../scoring/PROTOCOL.md). Formal ranking is prohibited until all seven canonical metrics, preregistered scenarios and 30 paired seeds are present, after oracle correctness gates. Classic baselines and paper reproductions remain explicitly distinguished.
