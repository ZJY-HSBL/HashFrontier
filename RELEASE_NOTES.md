# HashFrontier v0.8.0 — SODA 2025 Section 4 joins the unified trace comparison

- Adds independent implementation of **Basic Bubble-up Cuckoo (§4)** from Kuszmaul–Mitzenmacher, SODA 2025. Four evacuation rules, bounded core-eviction chain, explicit slot-choice metadata, and an out-of-theorem direct erase extension are documented.
- Adds randomized correctness testing (12 seeds × four loads), with check against `std::unordered_set`.
- Extends the SIGMOD ZombieHT / VLDB TPHT *author-source* common binary-trace runner to **six implementations across three papers**, 70/85/95% occupancy, ten common seeds (101–110), 32 churn cycles per trace, and 180 planned runs. Actual completion and measurements are in the post-release CI Artifact and its uploaded summary; a code release is **not** itself an experiment-completion claim.
- Preserves authors' core source SHA locks; our independent basic Bubble-up is **not** a reproduction of SODA advanced §5 or its main bound. No HF-Score until compatible seven-metric, 30-seed data and a verified, common memory accounting protocol exist.
- The earlier v0.7.2 three-seed data remain intact and separate. This release retains unsuccessful runs in the completion matrix rather than silently excluding them.
