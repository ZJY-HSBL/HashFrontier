#include "hashfrontier/advanced_bubble_appendix_c.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_set>
#include <vector>
static void ensure(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}
int main(){
  std::uint64_t checks=0, successful_rebuilds=0, observed_tombstone_move=0;
  for(auto cap:{256UL,1024UL,4096UL})for(auto load:{70,85,95})
  for(std::uint64_t seed=1;seed<=6;++seed){
    hf::AdvancedBubbleAppendixC ht(cap,10,5,.5,seed,load/100.0,
                                   (load+(100-load)*0.6)/100.0,4096);
    std::mt19937_64 rng(seed*371+cap*17+load);
    std::unordered_set<std::uint64_t> oracle;
    std::vector<std::uint64_t> live;
    const std::size_t n=cap*load/100;
    for(std::size_t i=0;i<n;i++) {
      const auto key=seed*1000000000000ULL+i*65537;
      if(!ht.insert(key)) throw std::runtime_error("build insertion failure");
      oracle.insert(key);live.push_back(key);
    }
    ensure(ht.valid()&&ht.size()==oracle.size(),"initial invariant");
    for(int cycle=0;cycle<28;++cycle){
      for(std::size_t j=0;j<std::max<std::size_t>(1,n/24);++j){
        const auto loc=std::size_t(rng()%live.size());
        const auto old=live[loc];
        ensure(ht.erase(old),"erase existing");oracle.erase(old);
        ensure(!ht.contains(old),"tombstone is not live");
        ensure(!ht.erase(old),"repeat deletion must fail");
        if(j%7==0){
          ensure(ht.insert(old),"revive tombstone");oracle.insert(old);
          ensure(ht.contains(old),"revived key not found");
          ensure(ht.erase(old),"delete revived key");oracle.erase(old);
        }
        const auto fresh=seed*1000000000000ULL + 9000000000ULL +
                          std::uint64_t(cycle)*cap*2+j;
        if(!ht.insert(fresh)) {std::cerr<<"Failure cap="<<cap<<" load="<<load<<" seed="<<seed<<" cycle="<<cycle<<" j="<<j<<" physical="<<ht.physical_size()<<" live="<<ht.size()<<" tomb="<<ht.tombstones()<<" rebuilds="<<ht.rebuilds()<<" failed_rebuilds="<<ht.failed_rebuilds()<<"\n";throw std::runtime_error("churn insertion failure");}
        oracle.insert(fresh);live[loc]=fresh;
        ensure(!ht.insert(fresh),"duplicate insertion must fail");
        ++checks;
      }
      ensure(ht.valid() && ht.size()==oracle.size(),"size / augmented placement");
      ensure(ht.physical_size()==ht.size()+ht.tombstones(),"tombstone accounting");
      for(auto key:live){ensure(ht.contains(key),"false negative");++checks;}
      for(int q=0;q<64;++q){auto key=rng();ensure(ht.contains(key)==bool(oracle.count(key)),"oracle query");++checks;}
      if(ht.tombstones()>0) ++observed_tombstone_move;
    }
    successful_rebuilds+=ht.rebuilds();
  }
  ensure(successful_rebuilds>0,"scheduled rebuild never triggered");
  ensure(observed_tombstone_move>0,"tombstone state never seen");
  std::cout<<"PASS Appendix C: "<<checks<<" checks, "<<successful_rebuilds
    <<" full rebuilds and "<<observed_tombstone_move<<" cycles with tombstones\n";
}
