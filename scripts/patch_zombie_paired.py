#!/usr/bin/env python3
"""Instrument *upstream benchmark input generation*, not hashing algorithms.

The author uses RAND_bytes() for constructing load/churn operations and rand()
for choice ordering. This opt-in experiment replaces RAND_bytes ONLY in the
benchmark TU with a seedable pseudorandom stream; then records hashes of the
materialized operation traces. It leaves src/gqf.c and all hash table internals
unchanged. Experiments using the patched benchmark are labeled "instrumented
upstream", NOT unmodified author-paper reproduction.
"""
from __future__ import annotations
import argparse
from pathlib import Path

PROLOGUE = r'''
// HashFrontier paired-input instrumentation (only in upstream bench/hm_churn.cc).
// We do NOT replace the author's core hash table implementation.
#include <random>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
static std::uint64_t hf_seed() {
  const char* p = std::getenv("HF_PAIR_SEED");
  if (!p || !*p) return 41;
  char* end = nullptr;
  auto n = std::strtoull(p, &end, 10);
  if (!end || *end) { std::abort(); }
  return n;
}
static int hf_rand_bytes(unsigned char* dst, int length) {
  // Byte order is explicitly specified; fixed seed => identical test inputs.
  static std::mt19937_64 rng(hf_seed());
  int i = 0;
  while (i < length) {
    const std::uint64_t word = rng();
    for (int j = 0; j < 8 && i < length; ++j, ++i)
      dst[i] = static_cast<unsigned char>((word >> (8 * j)) & 255U);
  }
  return 1;
}
// All other source files still use their original implementation / RNG.
'''
TRACE = r'''
static void hf_write_trace(const std::string& directory, const char* phase,
                           int cycle, const std::vector<hm_op>& ops) {
  std::uint64_t hash = 14695981039346656037ULL;
  auto append = [&](std::uint64_t v) {
    for (int b = 0; b < 8; ++b) {
      hash ^= (v >> (8 * b)) & 255ULL;
      hash *= 1099511628211ULL;
    }
  };
  append(static_cast<std::uint64_t>(ops.size()));
  for (const auto& op : ops) {
    append(static_cast<std::uint64_t>(op.op));
    append(static_cast<std::uint64_t>(op.key));
    append(static_cast<std::uint64_t>(op.value));
  }
  const bool first = std::string(phase) == "BUILD";
  std::ofstream output(directory + "paired-traces.csv",
                       first ? std::ios::trunc : std::ios::app);
  if (!output) { throw std::runtime_error("paired trace output open failed"); }
  if (first) output << "phase,cycle,operations,fnv64\n";
  output << phase << ',' << cycle << ',' << ops.size() << ',' << hash << '\n';
}
'''

def exactly_one(text: str, old: str, new: str) -> str:
    if text.count(old) != 1:
        raise ValueError(f'upstream drift: expected exactly one anchor {old[:100]!r}; found {text.count(old)}')
    return text.replace(old, new, 1)

def patch(s: str) -> str:
    if 'HashFrontier paired-input instrumentation' in s:
        raise ValueError('refusing to instrument already patched source')
    # The upstream version currently calls RAND_bytes multiple times in only
    # its workload generator. Reject changed source instead of silent fallback.
    count = s.count('RAND_bytes(')
    if count != 6:
        raise ValueError(f'expected 6 author RAND_bytes calls, found {count}')
    s = s.replace('RAND_bytes(', 'hf_rand_bytes(')
    s = PROLOGUE + '\n' + s
    s = exactly_one(s, '#include "hm_wrapper.h"', '#include "hm_wrapper.h"\n' + TRACE)
    s = exactly_one(s, '  generate_load_ops(ops, kv);',
                    '  generate_load_ops(ops, kv);\n  hf_write_trace(dir, "BUILD", -1, ops);')
    s = exactly_one(s, '    generate_churn_ops(ops, kv);',
                    '    generate_churn_ops(ops, kv);\n    hf_write_trace(dir, "CHURN", i, ops);')
    s = exactly_one(s, 'int main(int argc, char **argv) {',
                    'int main(int argc, char **argv) {\n  std::srand(static_cast<unsigned>(hf_seed()));')
    return s

def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument('upstream_benchmark', type=Path)
    args = p.parse_args()
    source = args.upstream_benchmark.read_text(encoding='utf8')
    changed = patch(source)
    args.upstream_benchmark.write_text(changed, encoding='utf8')
    print(f'Instrumented {args.upstream_benchmark} for verified paired source-level traces')

if __name__ == '__main__':
    main()
