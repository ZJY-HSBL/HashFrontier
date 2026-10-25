#include "hashfrontier/partner_ram_section3.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <vector>
using Clock=std::chrono::steady_clock;
static std::size_t value_for(const hf::PartnerRAMSection3& h,std::size_t w,std::size_t b){
 for(std::size_t v=0;v<h.word_alphabet();++v)if(h.destination_bin(w,v)==b)return v;
 throw std::runtime_error("bin value not representable");
}
int main(int argc,char**argv){
 const auto seeds=argc>1?std::stoul(argv[1]):3UL;
 const auto trials=argc>2?std::stoul(argv[2]):1500UL;
 std::cout<<"algorithm,bins,B,seed,attempts,committed,rejected,mutation_attempts,mutation_committed,noops,cross_bin_reuse,update_ns_per_attempt,encoded_words,valid\n";
 for(auto bins:{8UL,16UL,32UL})for(auto B:{16UL,32UL,64UL})for(std::size_t seed=1;seed<=seeds;++seed)
 for(auto method:{0,1}){
  hf::PartnerRAMSection3 h(bins,B,seed);
  std::vector<std::optional<std::size_t>> expected(h.num_words());
  const auto left=bins/2,right=left+1;
  for(std::size_t w=0;w<2*B;++w){
   auto v=value_for(h,w,w<B?left:right);
   if(!h.write_batch_reuse({{w,v}}))throw std::runtime_error("initial full-bin setup failed");
   expected[w]=v;
  }
  std::size_t accepted=0,mutations=0,changed=0,noops=0;
  const auto begin=Clock::now();
  for(std::size_t j=0;j<trials;++j){
   const auto a=j%B,b=B+j%B;
   const auto va=value_for(h,a,(j/B)%2==0?right:left);
   const auto vb=value_for(h,b,(j/B)%2==0?left:right);
   const bool is_mutation=expected[a]!=va || expected[b]!=vb;
   if(is_mutation)++mutations;else ++noops;
   const bool ok=method==0?h.write_batch_disjoint({{a,va},{b,vb}}):h.write_batch_reuse({{a,va},{b,vb}});
   if(ok){expected[a]=va;expected[b]=vb;++accepted;if(is_mutation)++changed;}
  }
  const auto end=Clock::now();
  if(!h.verify(expected))throw std::runtime_error("oracle/permutation mismatch");
  const auto stats=h.batch_stats();
  const double elapsed=std::chrono::duration<double,std::nano>(end-begin).count()/trials;
  std::cout<<(method==0?"disjoint_old":"prestate_reuse")<<','<<bins<<','<<B<<','<<seed<<','<<trials<<','<<accepted<<','<<(trials-accepted)<<','<<mutations<<','<<changed<<','<<noops<<','<<stats.reused_freed_partner<<','<<elapsed<<','<<h.encoded_words()<<",1\n";
 }
}
