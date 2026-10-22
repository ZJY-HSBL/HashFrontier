#include "hashfrontier/table.hpp"
#include <algorithm>
#include <stdexcept>
#include <cstdint>
#include <iostream>
#include <random>
#include <unordered_set>
#include <vector>

inline void check(bool ok) { if (!ok) throw std::runtime_error("differential mismatch"); }
template <typename T> void test(const char* name) {
    for (const std::size_t cap : {2UL, 8UL, 64UL, 1024UL}) {
        for (std::uint64_t seed = 0; seed < 6; ++seed) {
            T h(cap, seed);
            std::unordered_set<std::uint64_t> ref;
            std::mt19937_64 rng(seed * 1315423911ULL + cap);
            for (int op = 0; op < 12500; ++op) {
                auto key = rng() % (cap * 3 + 3);
                const auto command = rng() % 3;
                if (command == 0) {
                    const bool expected = ref.count(key) ? false : ref.size() < cap;
                    const bool actual = h.insert(key);
                    check(actual == expected);
                    if (actual) ref.insert(key);
                } else if (command == 1) {
                    const bool expected = ref.erase(key) != 0;
                    check(h.erase(key) == expected);
                } else {
                    check(h.contains(key) == (ref.count(key) != 0));
                }
                check(h.size() == ref.size());
                if ((op % 997) == 0) {
                    for (std::uint64_t k = 0; k < cap * 3 + 3; ++k)
                        check(h.contains(k) == (ref.count(k) != 0));
                }
            }
        }
    }
    std::cout << "PASS " << name << " (300,000 randomized operations)\n";
}
template <typename T> void wraparound_test() {
    T h(8, 17);
    std::vector<std::uint64_t> keys;
    for (std::uint64_t key = 0; keys.size() < 8; ++key)
        if ((hf::mix64(key ^ 17) & 7U) == 7U) keys.push_back(key);
    for (const auto key : keys) check(h.insert(key));
    check(!h.insert(0xffffffffffffffffULL));
    for (const auto key : keys) check(h.contains(key));
    for (std::size_t i = 0; i < keys.size(); i += 2) check(h.erase(keys[i]));
    for (std::size_t i = 0; i < keys.size(); ++i) check(h.contains(keys[i]) == (i % 2 != 0));
    for (std::size_t i = 0; i < keys.size(); i += 2) check(h.insert(keys[i]));
    check(h.size() == 8);
    for (const auto key : keys) check(h.contains(key));
    for (auto key : keys) check(h.erase(key));
    check(h.size() == 0);
}
int main() {
    test<hf::LinearTombstone>("linear_tombstone");
    test<hf::LinearBackshift>("linear_backshift");
    test<hf::RobinHood>("robin_hood");
    test<hf::LinearPeriodicRebuild<2>>("linear_rebuild_2pct");
    test<hf::LinearPeriodicRebuild<8>>("linear_rebuild_8pct");
    test<hf::LinearPeriodicRebuild<50, true>>("linear_rebuild_half_slack");
    test<hf::LinearBoundedRepair<8>>("bounded_repair_8");
    test<hf::LinearBoundedRepair<32>>("bounded_repair_32");
    wraparound_test<hf::LinearBoundedRepair<8>>();
    wraparound_test<hf::LinearBoundedRepair<32>>();
    std::cout << "All randomized differential and wraparound tests passed.\n";
}
