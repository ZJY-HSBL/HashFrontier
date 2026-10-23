# HashFrontier v0.9.2 — TPHT author-native reproduction evidence

- Pins TPHT authors' production C11 implementation to `3c39de26faba11bc1a037a1dfd24e3c633ac8987` and executes the upstream **full modular/CPU SIMD correctness test runner**, all executed modes passing.
- Runs the **actual authors' own latency and space benchmark drivers**, scaled to 2^10–2^17 keys / 64-bit value at 85% load and 50,000-key space sweep.
- Retains 128 latency CSV observations, 320 space CSV observations, original CI environment logs, upstream tests, SHA-256 provenance, and two data-derived SVGs.
- Keeps the tier honest: **L2-partial author-native reproduction**, not complete L2 (paper figures or scale), and not cross-paper L3. Does not compute HF-Score from incompatible historical measurements.
- No original algorithm code is invented or modified in this experiment.
