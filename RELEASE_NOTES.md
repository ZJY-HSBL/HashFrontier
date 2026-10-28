# HashFrontier v0.18.0 — STOC 2025 PartnerRAM retrieval-value encoding diagnostic

- Verified a finite 3-hash XOR retrieval control whose values are physically encoded in PartnerRAM swaps, **without** accessing the external key-to-word index for retrieval reads.
- Cloud run: https://github.com/ZJY-HSBL/HashFrontier/actions/runs/40624353966. 18/18 real trials complete; randomized verification 403,660 checks / 5,600 update operations; ASan/UBSan passed.
- Original CSV, actual environment, SHA-256 provenance, strict completeness audit and SVG plots are attached.
- **Not the DMPP06 dynamic succinct retrieval algorithm** used in Bender, Kuszmaul and Zhou (STOC 2025). Active keys, logical slots and masks still use external memory. Every update triggers a whole-XOR-layout rebuild; no asymptotic O(1) or metadata-free claim. This is not a complete paper reproduction and must not enter HF-Score.
