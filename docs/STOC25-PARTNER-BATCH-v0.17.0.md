# STOC 2025 — Algorithm 5: simultaneous partner-slot reuse (v0.17)
Paper: https://arxiv.org/html/2503.13628 — Bender, Kuszmaul and Zhou.

The older Algorithm-5 conditional staging code rejected a batch when one
update wanted to use a partner slot freed by another update in the SAME batch.
The revised code first reads all old words from the physical prestate; it then
reserves distinct targets among both currently empty self-loops and slots that
will be freed. It finally decouples ALL old partners before linking new pairs.
Unsatisfiable placements reject with the physical array unchanged.

Reproduced: this finite prestate, simulate-then-commit Algorithm-5 mechanism.
NOT REPRODUCED: DMPP06 succinct retrieval data structure, metadata-free table
(§4), dynamic resizing (§5), asymptotic constant-time guarantees. The
implementation still stores external logical-slot metadata and uses O(B) bin
scanning. It must not enter a whole-paper performance ranking.

Test the full-bin, two-way permutation swap (100 paired updates per
configuration, 3 x 3 capacities/bin sizes, four seeds), attempt failure and
unchanged-state semantics, and use ASan/UBSan. Cloud benchmark tests 54
configurations (2 methods x 3 bin counts x 3 bin sizes x 3 seeds, 1500
requests per run). Methods do NOT complete equivalent work: the legacy
method rejects all actual mutations; its no-op timings are not equivalent
throughput. Results, environment and SVG are committed automatically only
after the GitHub Actions job succeeds.

Run:
    g++ -std=c++20 -O2 -Iinclude tests/test_partner_batch_reuse.cpp -o /tmp/t
    /tmp/t
    g++ -std=c++20 -O3 -DNDEBUG -Iinclude src/bench_partner_batch_reuse.cpp -o /tmp/b
    /tmp/b 3 1500 > /tmp/observations.csv
    python3 scripts/report_partner_batch_v017.py /tmp/observations.csv /tmp/result
