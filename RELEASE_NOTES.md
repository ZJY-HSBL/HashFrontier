# HashFrontier v0.11.1 — independent GitHub cloud benchmark rendering fix

- Preserves all **v0.11.0 SODA 2025 Appendix B real source, correctness tests and measured 120-row container CSV** unchanged.
- Fixes the **independent GitHub Actions paper-reproduction workflow**: GitHub Ubuntu runners do not preinstall `matplotlib`; the benchmark summary now generates the same readable SVG plots using **Python's standard library only**.
- Makes 120/120 observed row completeness and oracle pass checks an unconditional prerequisite for cloud graphics and GitHub job success, without external pip dependencies.
- Publishes corrected SVG figures and retains the exact original CSV and provenance. Cloud performance results are exploratory and cannot be combined with formal physical-host HF-Score measurements.
- There are no new hashing mechanisms, algorithm innovations, or invented performance measurements in this patch release.
