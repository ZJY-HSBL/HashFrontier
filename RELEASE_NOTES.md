# HashFrontier v0.8.2 — Published three-paper measurements and honest failure analysis

- Publishes **actual** v0.8.1 fixed-trace results directly in the repository as CSV and real SVG figures, each with SHA provenance and GitHub Actions run ID.
- Dynamic experiment: **163 / 180 completed**. All five author/native and standard-library comparison implementations completed 30/30; SODA §4 with *non-paper direct deletion extension* completed 10/10 at 70%, 3/10 at 85%, 0/10 at 95%.
- Static paper-aligned experiment: **180 / 180 completed**. SODA §4 basic Bubble-up constructed at 95% in all ten seeds, confirming that the dynamic failures are not simply build failures.
- The §4 independent implementation is much slower in nanosecond timings than the author TPHT implementation; this is an engineering comparison, not a proof of intrinsic inferiority. This version neither implements the SODA advanced §5 result nor computes a formal HF-Score.
- README now embeds the actual SVG figures and full evidence-based, bilingual-method analysis; the previous 'planned 180 runs' language is superseded by verified measured counts.
- Disables automatically rerunning the archived v0.7 cross-paper experiment on every subsequent commit. It remains manually runnable for independent reproduction.
