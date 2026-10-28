#include "hashfrontier/dmpp06_first_level.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>
using Clock=std::chrono::steady_clock;
static double ns(Clock::time_point a,Clock::time_point b){return std::chrono::duration<double,std::nano>(b-a).count();}
int main(){
 std::cout<<"component,keys,bucket_bits,capacity_per_bucket,seed,inserted,overflow,hit_checks,miss_checks,build_ns_per_attempt,hit_ns_per_op,miss_ns_per_op,erase_ns_per_op,valid\n";
 std::uint64_t sink=0;
 for(unsigned bits:{6u,8u})for(int n:{4096,8192,16384})for(unsigned slack:{0u,10u,25u})for(int seed=1;seed<=4;++seed){
   std::size_t cap=(static_cast<std::size_t>(n)*(100+slack)+100*(std::size_t(1)<<bits)-1)/(100*(std::size_t(1)<<bits));
   std::mt19937 rng(seed*31+bits*123+n);
   hf::DMPP06FirstLevel t(bits,cap,static_cast<std::uint16_t>(rng()|1),static_cast<std::uint16_t>(rng()));
   std::vector<std::uint16_t> keys(65536);std::iota(keys.begin(),keys.end(),0);
   std::shuffle(keys.begin(),keys.end(),rng);
   std::vector<std::uint16_t> accepted,rejected;accepted.reserve(n);rejected.reserve(n);
   auto a=Clock::now();
   for(int i=0;i<n;++i){auto k=keys[i];if(t.insert(k,k^0x5a5a))accepted.push_back(k);else rejected.push_back(k);}
   auto b=Clock::now();
   if(!t.verify()||t.size()!=accepted.size())throw std::runtime_error("invalid table");
   auto c=Clock::now();
   for(auto k:accepted){auto v=t.lookup(k);if(!v||v->second!=(k^0x5a5a))throw std::runtime_error("hit mismatch");sink+=v->first;}
   auto d=Clock::now();
   for(int i=n;i<2*n;++i){if(t.lookup(keys[i]))throw std::runtime_error("miss mismatch");}
   auto e=Clock::now();
   for(auto k:accepted){if(!t.erase(k))throw std::runtime_error("erase mismatch");}
   auto f=Clock::now();
   if(t.size()||!t.verify())throw std::runtime_error("erase invariant");
   std::cout<<"DMPP06_first_filter_2.1_3.1,"<<n<<','<<bits<<','<<cap<<','<<seed<<','<<accepted.size()<<','<<rejected.size()<<','<<accepted.size()<<','<<n<<','<<ns(a,b)/n<<','<<ns(c,d)/accepted.size()<<','<<ns(d,e)/n<<','<<ns(e,f)/accepted.size()<<",1\n";
 }
 if(sink==UINT64_MAX)std::cerr<<sink;
}
