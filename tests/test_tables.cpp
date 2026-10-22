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
int main() {
    test<hf::LinearTombstone>("linear_tombstone");
    test<hf::LinearBackshift>("linear_backshift");
    test<hf::RobinHood>("robin_hood");
    std::cout << "All randomized differential tests passed.\n";
}
