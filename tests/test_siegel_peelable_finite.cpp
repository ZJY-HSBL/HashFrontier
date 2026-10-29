#include "hashfrontier/siegel_peelable_finite.hpp"
#include <iostream>
#include <stdexcept>
#include <set>
#include <cstdint>
#include <functional>
#include <vector>
static void check(bool b,const char* why){if(!b)throw std::runtime_error(why);}
int main(){
  std::uint64_t checks=0,certified_subsets=0,exact_distributions=0;
  for(std::size_t left:{16UL,24UL,32UL,64UL})for(std::size_t degree:{3UL,4UL})
    for(std::uint64_t seed=1;seed<=3;++seed){
      auto [g,attempts]=hf::SiegelFinitePeelable::find_certified(left,2*left,degree,4,seed);
      auto c=g.certify(4);
      check(c.k_peelable&&c.witness.empty(),"non-peelable graph");
      check(attempts>=1,"incorrect attempts");
      checks+=2;
      certified_subsets+=c.subsets_checked;
      for(std::size_t key=0;key<left;++key){
        std::set<std::uint16_t> distinct(g.neighbors(key).begin(),g.neighbors(key).end());
        check(distinct.size()==degree,"repeated right neighbor");
        check(g.evaluate(key)==g.evaluate(key),"unstable hash word");checks+=2;
      }
    }
  // An adversarial graph with more left vertices than right vertices cannot be
  // 4-peelable; certify must return a genuine bad subset as a witness.
  {hf::SiegelFinitePeelable g(8,3,3,5,8);
    auto c=g.certify(4);
    check(!c.k_peelable && c.witness.size()>=2,"negative certificate not detected");++checks;
  }
  // Check exact k-wise output law by enumerating *every* independent random
  // assignment of one bit to a small right-side table (2^10 assignments).
  auto [small,attempts]=hf::SiegelFinitePeelable::find_certified(5,10,2,3,0x12345,1000);
  check(small.certify(3).k_peelable,"tiny certificate");++checks;
  for(unsigned n=1;n<=3;++n){
    std::vector<unsigned> choice;
    std::function<void(unsigned,unsigned)> go=[&](unsigned at,unsigned need){
      if(!need){
        std::vector<unsigned> frequencies(1U<<n);
        for(std::uint64_t table=0;table<(1ULL<<10);++table){
          unsigned pattern=0;
          for(unsigned j=0;j<n;++j)
            pattern|=static_cast<unsigned>(small.evaluate_with_bitmask(choice[j],table))<<j;
          ++frequencies.at(pattern);
        }
        for(auto count:frequencies){
          check(count==(1U<<(10-n)),"exact joint output uniformity violated");
          ++exact_distributions;
        }
        return;
      }
      for(unsigned i=at;i+need<=5;++i){choice.push_back(i);go(i+1,need-1);choice.pop_back();}
    };
    go(0,n);
  }
  bool threw=false;try{hf::SiegelFinitePeelable bad(4,3,4,1,2);}catch(const std::invalid_argument&){threw=true;}
  check(threw,"invalid graph accepted");++checks;
  std::cout<<"PASS checks="<<checks<<" verified_nonempty_subsets="<<certified_subsets
           <<" exact_joint_uniformity_cells="<<exact_distributions
           <<" tiny_search_attempts="<<attempts<<"\n";
}
