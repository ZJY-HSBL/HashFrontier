#include "hashfrontier/advanced_bubble_up.hpp"
#include <unordered_set>
#include <random>
#include <iostream>
#include <stdexcept>
#include <vector>
static void req(bool b) {if(!b)throw std::runtime_error("advanced bubble-up invariant / oracle mismatch");}
int main(){
  std::size_t checked=0, completed=0, declined=0, transitions=0;
  for(auto cap:{128UL,256UL,1024UL,4096UL})
  for(auto seed:{1ULL,3ULL,7ULL,13ULL,23ULL,41ULL}) {
    // d=6, core=3 creates a genuinely multi-phase schedule at ~91.8%.
    hf::AdvancedBubbleUp ht(cap,6,3,.5,seed,512);
    std::mt19937_64 r(seed+cap*77);
    std::unordered_set<std::uint64_t> expected;
    std::vector<std::uint64_t> keys;
    auto initial=ht.active_choices();
    for(std::size_t i=0;i<cap*95/100;i++) {
      auto key=r();while(expected.count(key))key=r();
      bool inserted=ht.insert(key);
      req(ht.valid());
      if(!inserted) { for(auto x:expected){req(ht.contains(x));++checked;} ++declined;break; }
      expected.insert(key);keys.push_back(key);++completed;
      if(ht.active_choices()>initial)++transitions;
      if((i%17)==0) {
        for(auto x:keys){req(ht.contains(x));++checked;}
        for(int j=0;j<20;j++){auto q=r();req(ht.contains(q)==(expected.count(q)>0));++checked;}
      }
    }
    for(auto x:expected){req(ht.contains(x));++checked;}
    req(ht.size()==expected.size());
    // Invalid parameters must fail, not silently substitute basic algorithm.
    bool rejected=false;try{hf::AdvancedBubbleUp wrong(128,2,3,.5,1);}catch(const std::invalid_argument&){rejected=true;}req(rejected);
  }
  req(transitions>0);
  std::cout<<"PASS Section 5.2 Advanced Bubble-up mechanics: "<<completed
           <<" accepted, "<<declined<<" explicit insertion failures, "<<checked
           <<" oracle checks, "<<transitions<<" phase-advance observations\n";
}
