#include "hashfrontier/siegel_peelable_finite.hpp"
#include <chrono>
#include <iostream>
#include <vector>
#include <cstdint>
#include <stdexcept>
using Clock=std::chrono::steady_clock;
int main(){
  std::cout<<"component,domain,right_cells,degree,k,seed,search_attempts,certified_subsets,evaluations,hash_ns_per_eval,graph_min_bytes,random_table_bytes,checksum,valid\n";
  for(std::size_t left:{16UL,24UL,32UL,64UL})for(std::size_t d:{3UL,4UL})
    for(std::uint64_t seed=1;seed<=5;++seed){
      auto [h,attempts]=hf::SiegelFinitePeelable::find_certified(left,2*left,d,4,seed);
      auto certificate=h.certify(4);
      if(!certificate.k_peelable)throw std::runtime_error("invalid graph certificate");
      constexpr std::size_t count=120000;
      std::vector<std::uint16_t> queries(count);
      std::uint64_t x=seed+1;
      for(auto& query:queries){
        x=x*2862933555777941757ULL+3037000493ULL;
        query=static_cast<std::uint16_t>(x%left);
      }
      std::uint64_t checksum=0;
      auto start=Clock::now();
      for(auto q:queries)checksum+=(h.evaluate(q)^0x9e3779b97f4a7c15ULL);
      auto stop=Clock::now();
      const double ns=std::chrono::duration<double,std::nano>(stop-start).count()/count;
      std::cout<<"siegel_lemma2.6_finite,"<<left<<','<<2*left<<','<<d<<",4,"<<seed<<','<<attempts<<','
               <<certificate.subsets_checked<<','<<count<<','<<ns<<','<<h.graph_minimum_bytes()<<','
               <<h.random_table_bytes()<<','<<checksum<<",1\n";
    }
}
