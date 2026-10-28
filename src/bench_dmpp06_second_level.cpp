#include "hashfrontier/dmpp06_second_level.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>
using Clock=std::chrono::steady_clock;
static double ns(Clock::time_point a,Clock::time_point b){return std::chrono::duration<double,std::nano>(b-a).count();}
int main(){
 std::cout<<"component,bucket_bits,first_tiles,levels,keys,seed,inserted,overflow,erased,hit_ops,miss_ops,insert_ns_attempt,hit_ns_op,miss_ns_op,erase_ns_op,code_capacity,valid\n";
 std::uint64_t checksum=0;
 for(unsigned bits:{6u,8u})for(std::size_t tiles:{16UL,32UL,64UL})
 for(std::size_t n:{4096UL,8192UL,16384UL})for(std::uint64_t seed=1;seed<=4;++seed){
  hf::DMPP06SecondLevel h(bits,tiles,4,static_cast<std::uint16_t>((seed*73+7)|1),
                            static_cast<std::uint16_t>(seed*937),seed);
  std::mt19937_64 rng(seed*13337+bits*100+tiles+n);
  std::vector<std::uint16_t> candidates(65536);
  std::iota(candidates.begin(),candidates.end(),0);
  std::shuffle(candidates.begin(),candidates.end(),rng);
  std::unordered_map<std::uint16_t,std::pair<std::size_t,std::uint16_t>> oracle;
  oracle.reserve(n);
  std::size_t accepted=0,overflow=0;
  auto a=Clock::now();
  for(std::size_t i=0;i<n;++i){
   auto key=candidates[i];auto value=static_cast<std::uint16_t>(i^0xa5b7);
   auto st=h.insert(key,value);
   if(st==hf::DMPP06SecondLevel::Insert::inserted){
    ++accepted;auto f=h.lookup(key);if(!f||f->payload!=value)throw std::logic_error("insert lookup");
    oracle[key]={f->stable_hashcode,value};
   }else if(st==hf::DMPP06SecondLevel::Insert::overflow)++overflow;
   else throw std::logic_error("duplicate unique key");
  }
  auto b=Clock::now();
  if(accepted+overflow!=n||!h.verify())throw std::logic_error("build invariant");
  std::vector<std::uint16_t> live;live.reserve(accepted);
  for(auto const&[key,v]:oracle)live.push_back(key);
  std::shuffle(live.begin(),live.end(),rng);
  const std::size_t qops=10000;
  auto c=Clock::now();
  for(std::size_t i=0;i<qops;++i){
    auto key=live[i%live.size()];auto f=h.lookup(key);
    if(!f||f->stable_hashcode!=oracle.at(key).first||f->payload!=oracle.at(key).second)
      throw std::logic_error("hit mismatch");
    checksum+=f->payload;
  }
  auto d=Clock::now();
  auto e=Clock::now();
  for(std::size_t i=0;i<qops;++i){
    auto key=candidates[n+i%(65536-n)];
    if(h.lookup(key))throw std::logic_error("false membership");
  }
  auto f=Clock::now();
  std::size_t erased=0;
  auto g=Clock::now();
  for(std::size_t i=0;i<live.size();i+=4){
    if(!h.erase(live[i]))throw std::logic_error("delete failed");
    oracle.erase(live[i]);++erased;
  }
  auto k=Clock::now();
  if(!h.verify()||h.size()!=oracle.size())throw std::logic_error("delete invariant");
  for(const auto &[key,ref]:oracle){
    auto got=h.lookup(key);
    if(!got||got->stable_hashcode!=ref.first||got->payload!=ref.second)
      throw std::logic_error("stable code violation");
  }
  std::cout<<"dmpp06_second_tile,"<<bits<<','<<tiles<<",4,"<<n<<','<<seed<<','<<accepted<<','<<overflow<<','<<erased<<','<<qops<<','<<qops<<','<<ns(a,b)/n<<','<<ns(c,d)/qops<<','<<ns(e,f)/qops<<','<<ns(g,k)/erased<<','<<h.code_capacity()<<",1\n";
 }
 if(checksum==UINT64_MAX)std::cerr<<checksum;
}
