# HashFrontier v0.14.0 — STOC 2025 Partner Hashing §3.1 subroutine reproduction

- **Partial scope only:** implemented and tested paper Algorithms 1–3, explicit external logical-position oracle, XOR-masked RAM words encoded as index/partner key swaps.
- 136,125 local and GitHub CI checks; 69 finite-case unrepresentable writes intentionally rejected with old values unchanged. ASan/UBSan local test passed.
- Actual GitHub Actions cloud (Intel Xeon Platinum 8573C, Ubuntu 24.04, GCC 13.3.0): 45/45 runs verified with raw CSV, test log, compiler fingerprint, SHA-256, complete 9-row aggregate, real SVG.
- No claim to complete the STOC hash table's metadata-free property, load-factor guarantee, §4 fixed-size construction, §5 resizing or O(1) bound; no HF-Score.
- Real cloud run: https://github.com/ZJY-HSBL/HashFrontier/actions/runs/40180896046.
