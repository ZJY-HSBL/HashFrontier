# DMPP06 §2.2 finite GF256 quotient family, v0.23.0

Original: Demaine, Meyer auf der Heide, Pagh, Pătraşcu, [LATIN 2006 manuscript](https://erikdemaine.org/papers/PerfectHashing_Manuscript/paper.pdf), Section 2.2.

**Scope: isolated finite mechanism only, NOT Theorem 5, Siegel hashing, or full STOC 2025 Partner Hashing.**

v0.22.0 uses a 256-entry row-shift array and invertible odd-mod-256 affine maps per column. This revision evaluates a polynomial over GF(256) with k sampled coefficients to generate row shifts (k = 2, 8, 16), then uses independently sampled GF(256) field-affine permutations x -> a*x+b with a != 0 in each column. For k distinct rows, polynomial evaluations are uniformly k-wise independent. This is NOT the sublinear-space O(1)-evaluation high-independence Siegel family used in the paper's proof. Evaluation requires O(k) field operations, with no claim of high-probability concentration for Theorem 5.

Verified: GF inverses, all 65,536 positions mapped bijectively and inverted at 4 k values × 4 seeds × 4 bucket widths. For a degree-1 polynomial, a fixed distinct pair of rows is exhaustively checked against all 65,536 coefficient combinations. ASan/UBSan tests run a smaller 2,048-key loop, not the full 65,536 loop.

Measurement: v0.22 explicit vs k=2,8,16; 8/10/12 bucket bits, 1,024/4,096 keys, contiguous/uniform input, 5 seeds = 240 raw observations. Distinct methods are not timed as identical code; measure both collision-involved keys and hash throughput, including slower field-polynomial evaluation. b<n cases do NOT replicate §2.2 Transformation 4. Metadata is only this fixed-16-bit implementation, not asymptotic bit space.

[Raw data](../results/v0.23.0-dmpp06-poly/raw.csv) · [Summary](../results/v0.23.0-dmpp06-poly/summary.csv) · [Audit](../results/v0.23.0-dmpp06-poly/audit.json) · [Collision SVG](../figures/v0.23.0-dmpp06-poly-collisions.svg) · [Timing SVG](../figures/v0.23.0-dmpp06-poly-hash-cost.svg)

The cited DMPP06 third-tier high-performance dictionary and Partner Hashing fully implicit retrieval are still missing. No final HF-Score.
