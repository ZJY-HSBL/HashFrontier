#include "hashfrontier/dmpp06_three_level.hpp"
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
 std::cout<<"bucket_bits_first,bucket_bits_second,first_codes,first_tiles,third_capacity,seed,attempted,inserted_first,inserted_second,inserted_third,overflow,hit_queries,miss_queries,updates,erases,build_ns_per_attempt,hit_ns_per_op,miss_ns_per_op,update_ns_per_op,erase_ns_per_op,valid\n";
 volatile std::uint64_t sink=0;
 for(unsigned b1:{4u,6u})for(unsigned b2:{4u,6u})for(std::size_t fc:{2UL,4UL})
 for(std::size_t tiles:{2UL,4UL})for(std::size_t cap:{64UL,512UL})for(unsigned seed=1;seed<=3;++seed){
  hf::DMPP06ThreeLevel16 t(b1,fc,b2,tiles,2,cap,3,seed*47,5,seed*113,seed*2027);
  std::mt19937_64 rng(seed*91+b1*371+b2*57+fc*13+tiles*7+cap);
  std::vector<std::uint16_t> keys(65536);std::iota(keys.begin(),keys.end(),0);
  std::shuffle(keys.begin(),keys.end(),rng);
  constexpr std::size_t attempts=1600;
  std::vector<std::uint16_t> accepted,rejected;
  auto a=Clock::now();
  for(std::size_t i=0;i<attempts;++i){
   auto s=t.insert(keys[i],static_cast<std::uint16_t>(i));
   if(s==hf::DMPP06ThreeLevel16::Insert::overflow)rejected.push_back(keys[i]);
   else{if(s==hf::DMPP06ThreeLevel16::Insert::already_present)throw std::runtime_error("duplicate key");accepted.push_back(keys[i]);}
  }
  auto b=Clock::now();auto built=t.stats();
  if(!t.verify())throw std::runtime_error("build invariant");
  std::uint64_t checksum=0;auto c=Clock::now();
  for(int pass=0;pass<4;++pass)for(auto key:accepted){
   auto x=t.lookup(key);if(!x)throw std::runtime_error("missing key");checksum+=x->payload;
  }
  auto d=Clock::now();auto e=Clock::now();
  for(int pass=0;pass<4;++pass)for(std::size_t i=attempts;i<attempts+1024;++i)
   if(t.lookup(keys[i]))throw std::runtime_error("false membership");
  auto f=Clock::now();const auto updates=accepted.size()/4;auto g=Clock::now();
  for(std::size_t i=0;i<updates;++i)if(!t.update(accepted[i],static_cast<std::uint16_t>(i^0x3692)))
   throw std::runtime_error("failed update");
  auto h=Clock::now();if(!t.verify())throw std::runtime_error("update invariant");
  auto j=Clock::now();for(auto key:accepted)if(!t.erase(key))throw std::runtime_error("erase failed");
  auto k=Clock::now();
  if(t.size()||!t.verify())throw std::runtime_error("erase invariant");
  if(!rejected.empty()){
   if(t.lookup(rejected.front()))throw std::runtime_error("rejected key stored");
   if(t.insert(rejected.front(),42)==hf::DMPP06ThreeLevel16::Insert::overflow)
    throw std::runtime_error("freed capacity not reusable");
   if(!t.erase(rejected.front()))throw std::runtime_error("reinsert erase failed");
  }
  if(built.inserted_first+built.inserted_second+built.inserted_third!=accepted.size())
   throw std::runtime_error("admission counts differ");
  std::cout<<b1<<','<<b2<<','<<fc<<','<<tiles<<','<<cap<<','<<seed<<','<<attempts<<','
   <<built.inserted_first<<','<<built.inserted_second<<','<<built.inserted_third<<','<<rejected.size()
   <<','<<accepted.size()*4<<','<<4096<<','<<updates<<','<<accepted.size()
   <<','<<ns(a,b)/attempts<<','<<ns(c,d)/(accepted.size()*4)<<','<<ns(e,f)/4096
   <<','<<(updates?ns(g,h)/updates:0)<<','<<(accepted.empty()?0:ns(j,k)/accepted.size())<<",1\n";
  sink=checksum;
 }
 if(sink==UINT64_MAX)std::cerr<<sink;
}
