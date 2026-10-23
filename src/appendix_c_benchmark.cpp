// SODA 2025 Appendix C: finite-parameter regression workload on a faithful
// tombstone/rebuild *mechanism*, NOT author original figure reproduction.
#include "hashfrontier/advanced_bubble_appendix_c.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <unordered_set>
#include <vector>
using Clock=std::chrono::steady_clock;
static double ns(Clock::time_point a,Clock::time_point b){return std::chrono::duration<double,std::nano>(b-a).count();}
static double quantile(std::vector<double>& v,double q){
 if(v.empty()) throw std::runtime_error("missing latency distribution");
 const auto ix=std::size_t(std::ceil(q*v.size()))-1;
 std::nth_element(v.begin(),v.begin()+ix,v.end()); return v[ix];
}
int main(int argc,char**argv){
 const int nseeds=argc>1?std::stoi(argv[1]):10;
 const int cycles=argc>2?std::stoi(argv[2]):28;
 if(nseeds<1||cycles<1||nseeds>1000||cycles>10000) return 2;
 constexpr std::size_t cap=4096;
 std::cout<<"algorithm,load_pct,seed,capacity,cycles,operations,correct,build_ns_per_key,hit_ns_per_op,miss_ns_per_op,insert_ns_per_op,delete_ns_per_op,update_p99_ns,max_update_ns,rebuilds,failed_rebuilds,physical_entries,tombstones,bytes_per_key_impl_only\n";
 for(auto load:{70,85,95})for(int seed=1;seed<=nseeds;seed++){
  hf::AdvancedBubbleAppendixC ht(cap,10,5,.5,seed,load/100.,(load+(100-load)*.6)/100.,4096);
  std::mt19937_64 rng(seed*727ULL+load*413ULL);
  std::unordered_set<std::uint64_t> oracle;
  std::vector<std::uint64_t> live;
  const std::size_t n=cap*load/100;
  const std::uint64_t base=100000000000ULL*seed;
  const auto b0=Clock::now();
  for(std::size_t j=0;j<n;j++){
    auto k=base+j*65537;
    if(!ht.insert(k))throw std::runtime_error("build fail");
    live.push_back(k); oracle.insert(k);
  }
  const auto b1=Clock::now();
  std::vector<double> ins,del,update;
  ins.reserve(cycles*n/20);del.reserve(cycles*n/20);update.reserve(cycles*n/20*2);
  double hit_ns=0,miss_ns=0;std::uint64_t hcount=0,mcount=0;
  for(int c=0;c<cycles;c++) {
    const auto updates=std::max<std::size_t>(1,n/20);
    for(std::size_t j=0;j<updates;j++){
      const auto at=std::size_t(rng()%live.size());
      const auto old=live[at];
      const auto fresh=base+4000000000ULL+std::uint64_t(c)*cap*2+j;
      const auto d0=Clock::now();
      if(!ht.erase(old))throw std::runtime_error("erase failure");
      const auto d1=Clock::now();
      oracle.erase(old);
      if(ht.contains(old))throw std::runtime_error("deleted key returned true");
      const auto i0=Clock::now();
      if(!ht.insert(fresh))throw std::runtime_error("insert failure");
      const auto i1=Clock::now();
      oracle.insert(fresh);live[at]=fresh;
      auto d=ns(d0,d1), i=ns(i0,i1);
      del.push_back(d);ins.push_back(i);update.push_back(d);update.push_back(i);
    }
    for(int q=0;q<128;q++){
      const auto k=live[std::size_t(rng()%live.size())];
      auto a=Clock::now(); const bool hit=ht.contains(k);auto b=Clock::now();
      if(!hit)throw std::runtime_error("false negative");
      hit_ns+=ns(a,b);++hcount;
    }
    for(int q=0;q<128;q++){
      const auto k=base+9000000000ULL+std::uint64_t(c)*128+q;
      auto a=Clock::now(); const bool miss=ht.contains(k);auto b=Clock::now();
      if(miss)throw std::runtime_error("false positive");
      miss_ns+=ns(a,b);++mcount;
    }
    if(ht.size()!=oracle.size()||!ht.valid())throw std::runtime_error("set invariant failure");
    for(auto x:live)if(!ht.contains(x))throw std::runtime_error("final live key absent");
  }
  double isum=std::accumulate(ins.begin(),ins.end(),0.0), dsum=std::accumulate(del.begin(),del.end(),0.0);
  const auto p99=quantile(update,.99); const auto maxlat=*std::max_element(update.begin(),update.end());
  std::cout<<"advanced_appendix_c,"<<load<<','<<seed<<','<<cap<<','<<cycles<<','<<ins.size()<<",true,"
       <<ns(b0,b1)/n<<','<<hit_ns/hcount<<','<<miss_ns/mcount<<','<<isum/ins.size()<<','
       <<dsum/del.size()<<','<<p99<<','<<maxlat<<','<<ht.rebuilds()<<','<<ht.failed_rebuilds()<<','
       <<ht.physical_size()<<','<<ht.tombstones()<<','<<double(ht.memory_bytes())/ht.size()<<'\n';
 }
}
