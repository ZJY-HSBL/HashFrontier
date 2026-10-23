# HashFrontier v0.7.1 — native author adapter correctness fix

The v0.7.0 author-source experiment did NOT pass preflight: ZombieHT insertion returned nonnegative success code 2 but the new HashFrontier wrapper mistakenly rejected it. This version accepts r>=0 while rejecting negative failure codes and re-runs all shared binary traces, oracle checks and completeness auditing. Author algorithm code is unchanged.

No performance claim is valid until the NEW GitHub workflow and its raw Artifact are validated. Pilot: 70/85/95% load, 3 seeds, 32 churn cycles, five implementations. This preliminary set-membership comparison does not qualify for formal HF-Score due to key/value and memory-accounting differences.
