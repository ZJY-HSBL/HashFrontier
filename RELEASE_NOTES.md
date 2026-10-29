# HashFrontier v0.23.0 — finite DMPP06 §2.2 GF256 polynomial and field-affine model

- k-wise polynomial row shifts over GF(256) for finite k = 2, 8, 16, and independently seeded field-affine 2-independent column permutations.
- Full 65,536-element permutation/inverse verification; exhaustive two-row polynomial pair independence; CTest and bounded ASan/UBSan passed.
- [GitHub Actions 40841729087](https://github.com/ZJY-HSBL/HashFrontier/actions/runs/40841729087): 240/240 genuine experimental records, 48 groups (5 seeds), strict audit, CSV, SVG and SHA-256. Cloud machine AMD EPYC 9V74.
- Representative 4,096 shuffled keys, 1,024 buckets: v0.22 2.237 ns/hash and 1,024 B metadata; polynomial k=2/8/16 24.954 / 74.945 / 143.542 ns/hash and 770 / 776 / 784 B.
- **Not Siegel hashing, not DMPP06 Theorem 5, not full Partner Hashing; no HF-Score.** The finite polynomial has O(k) evaluation.

Source/limits: docs/STOC25-DMPP06-GF256-v0.23.0.md.
