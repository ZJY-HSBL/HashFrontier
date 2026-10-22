#include "hashfrontier/table.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Same fixed-capacity workload and SplitMix64 hash across all algorithms.
// No resize and no multithreading; this is an exploratory CPU microbenchmark.
using Clock = std::chrono::steady_clock;
volatile std::uint64_t checksum = 0;
struct Input {
    std::vector<std::uint64_t> insertions, hits, misses, post_hits, post_misses;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> churn;
};
Input make_input(std::size_t capacity, double alpha, std::uint64_t seed, const std::string& pattern, int queries) {
    Input w;
    const auto n = static_cast<std::size_t>(std::floor(capacity * alpha));
    const auto base = 100000000ULL + seed * 10000000ULL;
    w.insertions.resize(n);
    std::iota(w.insertions.begin(), w.insertions.end(), base);
    std::mt19937_64 rng(seed * 31 + 0x123456789ULL);
    std::shuffle(w.insertions.begin(), w.insertions.end(), rng);
    auto live = w.insertions;
    std::uniform_int_distribution<std::size_t> ui(0, n - 1);
    const auto hot_n = std::max<std::size_t>(1, n / 20);
    const auto hot_first = n - hot_n;
    std::uniform_int_distribution<std::size_t> hot(0, hot_n - 1);
    // Discrete Zipf approximation is generated outside timed loop.
    std::vector<double> weights(n);
    for (std::size_t i = 0; i < n; ++i) weights[i] = 1.0 / std::pow(i + 1.0, 1.10);
    std::discrete_distribution<std::size_t> zipf(weights.begin(), weights.end());
    auto index = [&]() -> std::size_t {
        if (pattern == "zipf") return zipf(rng);
        if (pattern == "hotspot_shift") return rng() % 100 < 90 ? hot_first + hot(rng) : ui(rng);
        return ui(rng);
    };
    for (int i = 0; i < queries; ++i) {
        w.hits.push_back(live[index()]);
        w.misses.push_back(base + 1000000ULL + static_cast<std::uint64_t>(i));
    }
    for (int i = 0; i < 1500; ++i) {
        const auto pos = ui(rng);
        const auto added = base + 2000000ULL + static_cast<std::uint64_t>(i);
        w.churn.emplace_back(live[pos], added);
        live[pos] = added;
    }
    for (int i = 0; i < queries; ++i) {
        // Reverse hotspot for post-churn to measure access shift under same structure.
        std::size_t pos = index();
        if (pattern == "hotspot_shift" && rng() % 100 < 90) pos = hot(rng);
        w.post_hits.push_back(live[pos]);
        w.post_misses.push_back(base + 3000000ULL + static_cast<std::uint64_t>(i));
    }
    return w;
}
void csv(const std::string& name, const std::string& pattern, std::uint64_t seed, double alpha,
         std::size_t capacity, std::size_t n, const char* phase, std::size_t ops,
         std::chrono::nanoseconds ns, hf::Stats stats, std::size_t bytes) {
    std::cout << name << ',' << pattern << ',' << seed << ',' << alpha << ',' << capacity << ','
              << n << ',' << phase << ',' << ops << ',' << ns.count() << ','
              << (double)ns.count() / ops << ',' << (double)stats.probes / ops << ','
              << (double)stats.moves / ops << ',' << bytes << '\n';
}
template <typename T> void run(const char* name, const Input& w, const std::string& pattern,
                               std::uint64_t seed, double alpha, std::size_t capacity) {
    T t(capacity, seed);
    const auto n = w.insertions.size();
    auto measure = [&](const char* phase, std::size_t ops, auto&& f) {
        t.reset_stats();
        const auto start = Clock::now();
        const std::uint64_t observed = f();
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start);
        checksum = checksum ^ observed;
        csv(name, pattern, seed, alpha, capacity, n, phase, ops, elapsed, t.stats(), t.bytes());
    };
    measure("build", n, [&] { std::uint64_t x = 0; for (auto k : w.insertions) x += t.insert(k); return x; });
    if (t.size() != n) throw std::logic_error("build mismatch");
    measure("hit", w.hits.size(), [&] { std::uint64_t x = 0; for (auto k : w.hits) x += t.contains(k); return x; });
    measure("miss", w.misses.size(), [&] { std::uint64_t x = 0; for (auto k : w.misses) x += t.contains(k); return x; });
    measure("churn", w.churn.size() * 2, [&] {
        std::uint64_t x = 0;
        for (auto [gone, added] : w.churn) { x += t.erase(gone); x += t.insert(added); }
        return x;
    });
    if (t.size() != n) throw std::logic_error("churn mismatch");
    measure("post_hit", w.post_hits.size(), [&] { std::uint64_t x = 0; for (auto k : w.post_hits) x += t.contains(k); return x; });
    measure("post_miss", w.post_misses.size(), [&] { std::uint64_t x = 0; for (auto k : w.post_misses) x += t.contains(k); return x; });
}
int main(int argc, char** argv) {
    const int queries = argc > 1 ? std::atoi(argv[1]) : 8000;
    const int num_seeds = argc > 2 ? std::atoi(argv[2]) : 5;
    if (queries < 1 || num_seeds < 1) return 2;
    constexpr std::size_t capacity = 4096;
    std::cout << "algorithm,pattern,seed,load,capacity,size,phase,operations,elapsed_ns,ns_per_op,probes_per_op,moves_per_op,allocated_bytes\n";
    for (std::uint64_t seed = 1; seed <= static_cast<std::uint64_t>(num_seeds); ++seed) {
        for (double alpha : {0.50, 0.80, 0.93}) {
            for (const auto& pattern : {std::string("uniform"), std::string("zipf"), std::string("hotspot_shift")}) {
                auto w = make_input(capacity, alpha, seed, pattern, queries);
                run<hf::LinearTombstone>("linear_tombstone", w, pattern, seed, alpha, capacity);
                run<hf::LinearBackshift>("linear_backshift", w, pattern, seed, alpha, capacity);
                run<hf::RobinHood>("robin_hood", w, pattern, seed, alpha, capacity);
            }
        }
    }
    std::cerr << "benchmark checksum=" << checksum << '\n';
}
