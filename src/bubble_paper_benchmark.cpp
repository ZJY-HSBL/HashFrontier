// SODA 2025 §4 Basic and §5.2 Advanced policy, same pure-insertion traces.
// Pilot engineering test; the finite seeds/core size do not certify Thm 5.1.
#include "hashfrontier/basic_bubble_up.hpp"
#include "hashfrontier/advanced_bubble_up.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_set>
using Clock=std::chrono::steady_clock;
using ns=std::chrono::duration<double,std::nano>;
struct Row{std::size_t ok=0, probes=0, failures=0;double build_ns=0, hit_ns=0, miss_ns=0;};
volatile std::uint64_t sink=0;
// Values distinct from sequential IDs, uniformly sampled from 64 bit space.
std::vector<std::uint64_t> keys(std::size_t n, std::uint64_t seed) {
  std::mt19937_64 rng(seed^0xDEADBEEF12345678ULL);
  std::unordered_set<std::uint64_t> used;std::vector<std::uint64_t> out;out.reserve(n);
  while(out.size()<n){auto k=rng();if(used.insert(k).second)out.push_back(k);}
  return out;
}
template<typename T>Row evaluate(T& t,const std::vector<std::uint64_t>& build,
                       const std::vector<std::uint64_t>& negative) {
  Row r;
  auto start=Clock::now();
  for(auto k:build){if(!t.insert(k)){++r.failures;break;}++r.ok;}
  auto stop=Clock::now();r.build_ns=ns(stop-start).count()/std::max<std::size_t>(1,r.ok);
  if(!t.valid())throw std::runtime_error("placement invariant failed");
  start=Clock::now();for(std::size_t i=0;i<r.ok;i++)if(!t.contains(build[i]))throw std::runtime_error("false negative");
  stop=Clock::now();r.hit_ns=ns(stop-start).count()/std::max<std::size_t>(1,r.ok);
  start=Clock::now();for(auto k:negative)if(t.contains(k))throw std::runtime_error("false positive");
  stop=Clock::now();r.miss_ns=ns(stop-start).count()/negative.size();
  sink=sink^r.ok;return r;
}
// Basic's invariants were separately tested in the v0.8 series.
Row evaluate_basic(hf::BasicBubbleUp& t,const std::vector<std::uint64_t>& build,
                   const std::vector<std::uint64_t>& negative) {
  Row r;auto start=Clock::now();
  try {for(auto k:build){if(!t.insert(k)){r.failures=1;break;}++r.ok;}}
  catch(const std::exception&){r.failures=1;}
  auto end=Clock::now();r.build_ns=ns(end-start).count()/std::max<std::size_t>(1,r.ok);
  start=Clock::now();for(std::size_t i=0;i<r.ok;i++)if(!t.contains(build[i]))throw std::runtime_error("basic false negative");
  end=Clock::now();r.hit_ns=ns(end-start).count()/std::max<std::size_t>(1,r.ok);
  start=Clock::now();for(auto k:negative)if(t.contains(k))throw std::runtime_error("basic false positive");
  end=Clock::now();r.miss_ns=ns(end-start).count()/negative.size();
  sink=sink^r.ok;return r;
}
int main(int argc,char**argv){
  const int seeds=argc>1?std::stoi(argv[1]):10;
  const std::size_t capacity=argc>2?std::stoul(argv[2]):8192;
  if(seeds<1||capacity<128)return 2;
  std::cout<<"method,load,seed,capacity,requested,inserted,status,build_ns_per_key,hit_ns_per_op,miss_ns_per_op,bytes_per_key,active_d,phase_count\n";
  std::cout<<std::fixed<<std::setprecision(5);
  for(int load:{70,85,95})for(int seed=101;seed<101+seeds;seed++){
    const auto requested=capacity*load/100;
    const auto initial=keys(requested,static_cast<std::uint64_t>(seed));
    const auto absent=keys(1024,static_cast<std::uint64_t>(seed)+999999);
    for(auto kind:{std::string("bubble_basic_s4"),std::string("bubble_advanced_s52")}){
      Row r;std::size_t bytes=0,d=0,ph=1;
      if(kind=="bubble_basic_s4"){
        hf::BasicBubbleUp ht(capacity,load,seed);r=evaluate_basic(ht,initial,absent);
        bytes=ht.memory_bytes();d=ht.choices();
      } else {
        // Fixed finite d=8,d_core=4 parameters: NOT guaranteed Thm 5.1.
        hf::AdvancedBubbleUp ht(capacity,8,4,.5,seed,2048);r=evaluate(ht,initial,absent);
        bytes=ht.memory_bytes();d=ht.active_choices();ph=ht.phases();
      }
      std::cout<<kind<<','<<load<<','<<seed<<','<<capacity<<','<<requested<<','<<r.ok<<','
               <<(r.ok==requested?"complete":"failed")<<','<<r.build_ns<<','<<r.hit_ns<<','
               <<r.miss_ns<<','<<(double(bytes)/std::max<std::size_t>(r.ok,1))<<','<<d<<','<<ph<<'\n';
    }
  }
  std::cerr<<"checksum="<<sink<<'\n';
}
