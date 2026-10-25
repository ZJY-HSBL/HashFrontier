// STOC 2025 Algorithm 4/5 conditional retrieval; finite cloud pilot only.
#include "hashfrontier/partner_retrieval_section3.hpp"
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>
using Clock=std::chrono::steady_clock;
int main(int argc,char**argv){
 const int seeds=argc>1?std::atoi(argv[1]):5;
 if(seeds<1||seeds>50)throw std::invalid_argument("seed count");
 std::cout<<"variant,bins,bin_size,seed,keys,attempts,committed,rejected,update_ns_per_batch,query_ns_per_op,planned_swaps,prestate_reads,physical_bytes,external_metadata_min_bytes,valid\n";
 for(auto bins:{8UL,16UL,32UL,64UL})for(auto B:{16UL,32UL,64UL})
 for(int seed=1;seed<=seeds;++seed){
  std::vector<std::uint64_t> keys;
  for(std::size_t i=0;i<bins*B/8;++i)keys.push_back(std::uint64_t(seed)*10000000+i+1);
  hf::PartnerRetrievalSection3 h(bins,B,seed,keys);
  std::unordered_map<std::uint64_t,std::optional<std::size_t>> oracle;
  for(auto k:keys)oracle[k]=std::nullopt;
  std::mt19937_64 rng(seed*17+bins*71+B);
  constexpr int attempts=12000,queries=100000;
  int accepted=0,failed=0;
  auto t0=Clock::now();
  for(int j=0;j<attempts;++j){
   const auto i=rng()%keys.size();
   const auto other=(i+1+rng()%(keys.size()-1))%keys.size();
   auto a=rng()%3?std::optional<std::size_t>(rng()%(bins/2)):std::nullopt;
   auto b=rng()%3?std::optional<std::size_t>(rng()%(bins/2)):std::nullopt;
   const bool ok=h.update_retrieval({{keys[i],a},{keys[other],b}});
   if(ok){++accepted;oracle[keys[i]]=a;oracle[keys[other]]=b;}
   else ++failed;
  }
  auto t1=Clock::now();
  std::uint64_t accumulator=0;
  for(int j=0;j<queries;++j){
   auto key=keys[rng()%keys.size()];
   auto found=h.query_retrieval(key);
   if(found!=oracle.at(key))throw std::runtime_error("retrieval query differs from oracle");
   accumulator+=found?*found+1:0;
  }
  auto t2=Clock::now();
  if(!h.verify(oracle))throw std::runtime_error("permutation failure");
  auto s=h.batch_stats();
  if(s.committed!=unsigned(accepted)||s.rejected!=unsigned(failed))
   throw std::runtime_error("batch status mismatch");
  double update=std::chrono::duration<double,std::nano>(t1-t0).count()/attempts;
  double query=std::chrono::duration<double,std::nano>(t2-t1).count()/queries;
  std::cout<<"conditional_alg4_5,"<<bins<<','<<B<<','<<seed<<','<<keys.size()<<','
           <<attempts<<','<<accepted<<','<<failed<<','<<update<<','<<query<<','
           <<s.planned_swaps<<','<<s.prestate_reads<<','<<h.physical_bytes()<<','
           <<h.external_metadata_min_bytes()<<",1\n";
  if(accumulator==UINT64_MAX)std::cerr<<accumulator;
 }
}
