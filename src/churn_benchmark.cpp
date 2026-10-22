#include "hashfrontier/table.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <map>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Experimental trace benchmark: same mutation/lookup order per algorithm and seed.
// Fixed capacity, no resizing, each cycle = delete/insert pairs + positive/negative lookups.
// P99 time includes two clock calls per pair; it is an exploratory latency indicator.
using Clock = std::chrono::steady_clock;
struct Workload {
    std::vector<std::uint64_t> initial;
    struct Cycle { std::vector<std::pair<std::uint64_t, std::uint64_t>> updates;
                   std::vector<std::uint64_t> hits, misses; };
    std::vector<Cycle> cycles;
};
static Workload make_workload(std::size_t capacity, int load_pct, std::uint64_t seed,
                              int cycles, int queries) {
    Workload w;
    const std::size_t n = capacity * static_cast<std::size_t>(load_pct) / 100;
    const std::uint64_t start = 1000000000ULL + seed * 10000000ULL;
    w.initial.resize(n);
    std::iota(w.initial.begin(), w.initial.end(), start);
    std::mt19937_64 rng(seed * 1337 + 0x137ULL);
    std::shuffle(w.initial.begin(), w.initial.end(), rng);
    auto live = w.initial;
    const std::size_t changes = std::max<std::size_t>(1, n / 20);
    for (int c = 0; c < cycles; ++c) {
        Workload::Cycle ops;
        std::uniform_int_distribution<std::size_t> index(0, n - 1);
        for (std::size_t k = 0; k < changes; ++k) {
            auto i = index(rng);
            auto fresh = start + 1000000ULL + static_cast<std::uint64_t>(c) * capacity + k;
            ops.updates.emplace_back(live[i], fresh);
            live[i] = fresh;
        }
        for (int q = 0; q < queries; ++q) {
            ops.hits.push_back(live[index(rng)]);
            ops.misses.push_back(start + 100000000ULL + static_cast<std::uint64_t>(c) * queries + q);
        }
        w.cycles.push_back(std::move(ops));
    }
    return w;
}
template<typename T> static double percentile(std::vector<T> vals, double q) {
    if (vals.empty()) throw std::logic_error("empty percentile");
    const std::size_t idx = static_cast<std::size_t>(std::ceil(q * vals.size())) - 1;
    std::nth_element(vals.begin(), vals.begin() + idx, vals.end());
    return static_cast<double>(vals[idx]);
}
static volatile std::uint64_t consumed = 0;
static std::uint64_t quantile(const std::map<std::uint64_t, std::uint64_t>& hist,
                              std::uint64_t count, std::uint64_t numerator, std::uint64_t denominator) {
    const auto rank = (count * numerator + denominator - 1) / denominator;
    std::uint64_t cumulative = 0;
    for (const auto& [value, frequency] : hist) {
        cumulative += frequency;
        if (cumulative >= rank) return value;
    }
    throw std::logic_error("invalid histogram");
}
template <typename T>
static void run(const char* name, const Workload& w, int load_pct, std::uint64_t seed,
                std::size_t cap, std::ostream* tails) {
    T ht(cap, seed);
    std::map<std::uint64_t, std::uint64_t> histogram;
    std::uint64_t total_updates = 0, total_rebuilds = 0;
    for (const auto key : w.initial)
        if (!ht.insert(key)) throw std::logic_error("initial load failed");
    for (std::size_t cycle = 0; cycle < w.cycles.size(); ++cycle) {
        const auto& ops = w.cycles[cycle];
        ht.reset_stats();
        std::vector<double> update_ns, update_probes;
        update_ns.reserve(ops.updates.size());
        update_probes.reserve(ops.updates.size());
        for (const auto& [old_key, new_key] : ops.updates) {
            const auto p0 = ht.stats().probes;
            const auto before = Clock::now();
            if (!ht.erase(old_key) || !ht.insert(new_key))
                throw std::logic_error("mutation correctness failed");
            const auto after = Clock::now();
            update_ns.push_back(std::chrono::duration<double, std::nano>(after - before).count());
            const auto delta = ht.stats().probes - p0;
            update_probes.push_back(static_cast<double>(delta));
            if (tails) { ++histogram[delta]; ++total_updates; }
        }
        const auto update_stats = ht.stats();
        total_rebuilds += update_stats.rebuilds;
        if (ht.size() != w.initial.size()) throw std::logic_error("load changed");
        ht.reset_stats();
        for (auto key : ops.hits) if (!ht.contains(key)) throw std::logic_error("hit failure");
        const double hit = double(ht.stats().probes) / ops.hits.size();
        ht.reset_stats();
        for (auto key : ops.misses) if (ht.contains(key)) throw std::logic_error("miss failure");
        const double miss = double(ht.stats().probes) / ops.misses.size();
        double avg_update_probes = std::accumulate(update_probes.begin(), update_probes.end(), 0.0) / update_probes.size();
        double avg_update_ns = std::accumulate(update_ns.begin(), update_ns.end(), 0.0) / update_ns.size();
        std::cout << name << ',' << load_pct << ',' << seed << ',' << cycle + 1 << ','
                  << cap << ',' << ht.size() << ',' << ops.updates.size() << ',' << hit << ',' << miss << ','
                  << avg_update_probes << ',' << percentile(update_probes, .99) << ','
                  << *std::max_element(update_probes.begin(), update_probes.end()) << ','
                  << avg_update_ns << ',' << percentile(update_ns, .99) << ','
                  << *std::max_element(update_ns.begin(), update_ns.end()) << ','
                  << update_stats.moves << ',' << update_stats.rebuilds << ',' << ht.bytes() << '\n';
        consumed = consumed ^ static_cast<std::uint64_t>(miss + hit);
    }
    if (tails) {
        *tails << name << ',' << load_pct << ',' << seed << ',' << total_updates << ','
               << quantile(histogram,total_updates,50,100) << ','
               << quantile(histogram,total_updates,95,100) << ','
               << quantile(histogram,total_updates,99,100) << ','
               << quantile(histogram,total_updates,999,1000) << ','
               << histogram.rbegin()->first << ',' << total_rebuilds << '\n';
    }
}
int main(int argc, char** argv) {
    const int seeds = argc > 1 ? std::stoi(argv[1]) : 5;
    const int cycles = argc > 2 ? std::stoi(argv[2]) : 12;
    const int queries = argc > 3 ? std::stoi(argv[3]) : 256;
    if (seeds < 1 || cycles < 1 || queries < 1) return 2;
    std::ofstream tail_file;
    std::ostream* tails = nullptr;
    if (argc > 4) {
        tail_file.open(argv[4]);
        if (!tail_file) throw std::runtime_error("cannot open tail output");
        tails = &tail_file;
        *tails << "algorithm,load_pct,seed,update_pairs,p50_probes,p95_probes,p99_probes,p999_probes,max_probes,total_rebuilds\n";
    }
    constexpr std::size_t capacity = 4096;
    std::cout << "algorithm,load_pct,seed,cycle,capacity,size,update_pairs,hit_probes_per_op,miss_probes_per_op,update_probes_per_pair,p99_update_probes,max_update_probes,pair_mean_ns,p99_pair_ns,max_pair_ns,moves,rebuilds,steady_slot_bytes\n";
    for (int load : {75, 85, 95}) for (int seed = 1; seed <= seeds; ++seed) {
        const auto w = make_workload(capacity, load, static_cast<std::uint64_t>(seed), cycles, queries);
        run<hf::LinearTombstone>("linear_tombstone", w, load, seed, capacity, tails);
        run<hf::LinearBackshift>("linear_backshift", w, load, seed, capacity, tails);
        run<hf::RobinHood>("robin_hood", w, load, seed, capacity, tails);
        run<hf::LinearPeriodicRebuild<2>>("linear_rebuild_2pct", w, load, seed, capacity, tails);
        run<hf::LinearPeriodicRebuild<8>>("linear_rebuild_8pct", w, load, seed, capacity, tails);
        run<hf::LinearPeriodicRebuild<50, true>>("linear_rebuild_half_slack", w, load, seed, capacity, tails);
        run<hf::LinearBoundedRepair<8>>("bounded_repair_8", w, load, seed, capacity, tails);
        run<hf::LinearBoundedRepair<32>>("bounded_repair_32", w, load, seed, capacity, tails);
    }
    std::cerr << "checksum=" << consumed << '\n';
}
