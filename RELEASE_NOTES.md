# HashFrontier v0.21.0 — DMPP06 three-tier finite cloud reproduction

- LATIN 2006 DMPP06 Section 3: route insert/query/update/delete through first/second/third tiers; preserve stable disjoint codes; explicit no-space failure.
- Third tier uses a standard-library map with free-code recycling; not the theoretical compact high-performance dictionary.
- GitHub Actions Ubuntu 24.04, GCC 13.3.0, Intel Xeon Platinum 8573C virtual CPU: 96 genuine measurements, 32 fully audited parameter groups.
- 115,200 randomized operations; 676,373 checks; 255,374 stable-code checks; AddressSanitizer and UndefinedBehaviorSanitizer passed.
- Actual CSV, summaries, provenance SHA-256, environment, tests and SVG are included in release assets.
- NOT complete DMPP06, NOT complete STOC25 Partner Hashing, no final HF-Score.

See docs/STOC25-DMPP06-THREE-LEVEL-v0.21.0.md for exact scope.
