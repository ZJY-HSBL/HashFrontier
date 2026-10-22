# HashFrontier v0.6.0 — HF-Score v1.0 unified evaluation standard

**2026-10-22. Protocol and software release, NOT new cross-paper replication evidence.**

This version freezes the proposed seven-metric dynamic CPU table scoring formula and adds a strict machine-readable evaluator, preregistered pilot scenario manifest, 12 pure-stdlib correctness/completeness tests, CI smoke, a demonstrably synthetic CLI fixture, and a review of prior measurement gaps.

Core metrics and fixed weights: initial construction 15%; successful lookup 20%; unsuccessful lookup 15%; insertion 15%; deletion 10%; per-operation mutation P99 10%; complete bytes per live key 15%. All are strictly positive lower-is-better. Across prerecorded scenario/seed pairs, score `100 * exp(mean_log_gain - 0.25 * between_scenario_log_gain_sd)` relative to pinned Robin Hood baseline. Missing metrics, failed correctness, different hardware or input trace, absent rows, and incomplete reference all fail the strict scoring gate. 30 seeds and paired seed bootstrap are required for formal candidate uncertainty intervals. Different capabilities/tracks are not silently mixed.

**No formal score ranking is published.** Existing v0.1–v0.5 results cannot support the seven-metric, cross-paper comparison and must not be merged or imputed. `examples/hfscore/*` is explicitly fake software-test data, not experimental performance. Current work remains reproducing eligible research papers and measuring them on the same trace.

Evidence: `results/v0.6.0-data-readiness.json`, `scoring/protocol-v1.0.json`, `scoring/PROTOCOL.md`, strict unit/CI tests. v0.5.0 crash and original ZombieHT replication limitations remain unresolved in this release.
