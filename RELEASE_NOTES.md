# HashFrontier v0.7.0 — first author-source cross-paper comparison

This version integrates the **unmodified native algorithm cores** of SIGMOD 2025 Zombie Hashing and PVLDB 2026 Tiny Pointer Hash Tables in one reproducible, oracle-validated **set-membership** benchmark. The common deterministic binary traces are archived with the workflow artifact. This release contains the benchmark source and protocol, not a claim that any method is faster in advance of runtime verification.

Five implementations: GZHM, GZHM_ADAPTIVE, Chained-TPHT, Flattened-TPHT and standard-library control. Pilot: three loads (70/85/95%), three seeds (101/102/103), 32 update cycles with successful/unsuccessful queries = 45 scheduled jobs. Strict per-run completion CSV, provenance and raw logs are mandatory.

**Important limitations:** Different native memory allocation accounting, TPHT 1-byte dummy payload vs Zombie 0-bit payload, CPU microbenchmark overhead and only three seeds make HF-Score v1.0 inapplicable. No fabricated composite ranking and no claim of Bubble-up Cuckoo replication. The author-source workflow will attach measured CSV and SVG after successful execution. Failed or incomplete runs remain recorded as such.
