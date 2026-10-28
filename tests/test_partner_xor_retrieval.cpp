#include "hashfrontier/partner_xor_retrieval.hpp"
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_map>
static void check(bool x, const char* label) {if(!x)throw std::runtime_error(label);}
int main(){
 std::uint64_t checks=0, operations=0, rebuilds=0;
 for(auto bins:{64UL,128UL})for(auto B:{32UL,64UL})for(std::uint64_t seed=1;seed<=4;++seed){
  hf::PartnerXorRetrieval t(bins,B,seed);
  std::unordered_map<std::uint64_t,std::size_t> oracle;
  std::mt19937_64 rng(seed*7919+bins*17+B);
  const auto limit=t.word_count()/7;
  for(std::size_t op=0;op<350;++op){
   auto key=1+rng()%(limit+10);
   if(rng()%4==0){
    const bool expected=oracle.erase(key)>0;
    const bool actual=t.erase(key);
    check(actual==expected,"erase return");
   }else{
    const auto value=static_cast<std::size_t>(rng()%t.alphabet());
    if(oracle.size()<limit || oracle.contains(key)){
      check(t.upsert(key,value),"unexpected rebuild rejection");
      oracle[key]=value;
    }
   }
   check(t.size()==oracle.size(),"size mismatch");++checks;
   for(auto[k,v]:oracle){check(t.retrieve(k)==v,"XOR retrieval mismatch");++checks;}
   if(op%29==0){check(t.verify(),"global verification");++checks;}
   ++operations;
  }
  // Failed insertion due to occupancy limit must leave all values unchanged.
  const auto before=oracle;
  for(std::uint64_t i=100000;i<100000+t.word_count();++i){
   if(!t.upsert(i,i%t.alphabet()))break;
   oracle[i]=i%t.alphabet();
  }
  check(t.verify(),"failure changed existing keys");
  for(auto[k,v]:oracle){check(t.retrieve(k)==v,"failure retained wrong value");++checks;}
  check(t.size()<=t.word_count()/5,"occupancy limit violated");
  rebuilds+=t.stats().successful_rebuilds;
 }
 std::cout<<"pass checks="<<checks<<" updates="<<operations<<" rebuilds="<<rebuilds<<"\n";
}
