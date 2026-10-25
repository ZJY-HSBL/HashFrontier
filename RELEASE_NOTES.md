# HashFrontier v0.16.0 — STOC 2025 Algorithms 4–5 partial reproduction

- Added conditional read-via-encoded-RAM retrieval and deferred disjoint pair relocation (prestate snapshot followed by commit). No modification or invention of a new hash-table algorithm.
- Preserved upstream Section 3 primitives and existing Section 3.2 conditional updates.
- GitHub CMake tests, randomized oracle, over-capacity atomic-rejection regression, ASan/UBSan all passed.
- **60/60 real GitHub Actions cloud observations**, 720,000 two-word batch attempts and 6,000,000 queries; archives contain the original CSV, provenance, SHA-256 checksums, summary, and measured SVG plots.
- **Not** the paper's full DMPP06 succinct retrieval, fixed-full table, or dynamic resizing. Current implementation uses external metadata and has no paper O(1) performance guarantee. No HF-Score.
- [Cloud evidence](https://github.com/ZJY-HSBL/HashFrontier/actions/runs/40184160034) · [Detailed report](docs/STOC25-PARTNER-RESULTS-v0.16.0.md).
