#include "hashfrontier/partner_retrieval_section3.hpp"
#include <cstdint>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>
static void check(bool b,const char*m){if(!b)throw std::runtime_error(m);}
int main(){
 std::uint64_t checks=0,committed=0,rejected=0;
 for(auto bins:{8UL,16UL,32UL,64UL})for(auto B:{16UL,32UL,64UL})
 for(std::uint64_t seed=1;seed<=5;++seed){
  std::vector<std::uint64_t> keys;
  for(std::size_t i=0;i<bins*B/8;++i)keys.push_back(seed*10000000ULL+i+1);
  hf::PartnerRetrievalSection3 h(bins,B,seed,keys);
  std::unordered_map<std::uint64_t,std::optional<std::size_t>> oracle;
  for(auto k:keys)oracle[k]=std::nullopt;
  std::mt19937_64 rng(seed*13+bins+B);
  for(int step=0;step<250;++step){
   const auto ia=rng()%keys.size();
   const auto ib=(ia+1+rng()%(keys.size()-1))%keys.size();
   auto a=rng()%4?std::optional<std::size_t>(rng()%(bins/2)):std::nullopt;
   auto b=rng()%4?std::optional<std::size_t>(rng()%(bins/2)):std::nullopt;
   const bool ok=h.update_retrieval({{keys[ia],a},{keys[ib],b}});
   if(ok){oracle[keys[ia]]=a;oracle[keys[ib]]=b;++committed;}
   else ++rejected;
   for(auto[k,v]:oracle){check(h.query_retrieval(k)==v,"retrieval mismatch");++checks;}
   if(step%40==0){check(h.verify(oracle),"permutation mismatch");++checks;}
  }
  check(h.verify(oracle),"final permutation");++checks;
 }
 {hf::PartnerRAMSection3 r(8,16,3);
  std::vector<std::pair<std::size_t,std::optional<std::size_t>>> impossible;
  for(std::size_t w=0;w<17;++w)
    for(std::size_t v=0;v<r.word_alphabet();++v)
      if(r.destination_bin(w,v)==4){impossible.emplace_back(w,v);break;}
  check(impossible.size()==17,"failed capacity fixture");
  check(!r.write_batch(impossible),"oversubscribed must reject");
  std::vector<std::optional<std::size_t>> empty(r.num_words());
  check(r.verify(empty),"failed batch modified physical state");
  checks+=3;
 }
 std::cout<<"STOC25 Alg4-5 restricted OK checks="<<checks<<" committed="<<committed<<" rejected="<<rejected<<"\n";
}
