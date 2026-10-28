#include "hashfrontier/dmpp06_second_level.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>
static void require(bool ok,const char* what){if(!ok)throw std::runtime_error(what);}
int main(){
 std::uint64_t checks=0,inserted=0,overflow=0,deleted=0;
 for(unsigned bits:{6u,8u})for(std::size_t first:{8UL,16UL,32UL,64UL})
 for(std::uint64_t seed=1;seed<=8;++seed){
  hf::DMPP06SecondLevel h(bits,first,3,7,11,seed); // deliberately small filter to force 3rd-level overflow
  std::mt19937_64 rng(seed*233+bits*111+first);
  std::vector<std::uint16_t> keys(65536);
  std::iota(keys.begin(),keys.end(),0);std::shuffle(keys.begin(),keys.end(),rng);
  std::unordered_map<std::uint16_t,std::pair<std::size_t,std::uint16_t>> oracle;
  for(int i=0;i<1000;++i){
   const auto key=keys[i];const auto value=static_cast<std::uint16_t>(rng());
   const auto status=h.insert(key,value);
   if(status==hf::DMPP06SecondLevel::Insert::inserted){
    auto got=h.lookup(key);require(got && got->payload==value,"missing inserted");
    oracle[key]={got->stable_hashcode,value};++inserted;
   }else{require(status==hf::DMPP06SecondLevel::Insert::overflow,"unexpected existing");
    require(!h.lookup(key),"overflow entered structure");++overflow;}
   ++checks;
  }
  require(h.verify(),"initial invariant");++checks;
  for(auto const& [k,v]:oracle){
   auto got=h.lookup(k);require(got && got->stable_hashcode==v.first && got->payload==v.second,"unstable code");++checks;
  }
  for(int i=0;i<1000;i+=3){
   const auto key=keys[i];if(!oracle.contains(key))continue;
   const auto code=oracle.at(key).first;
   require(h.update(key,0xa551),"update failed");++checks;
   require(h.lookup(key)->stable_hashcode==code,"update changed code");++checks;
   oracle[key].second=0xa551;
   require(h.erase(key),"erase failed");++checks;
   require(!h.lookup(key),"erase visible");++checks;
   oracle.erase(key);++deleted;
  }
  for(int i=1000;i<1400;++i){
   auto key=keys[i];auto status=h.insert(key,static_cast<std::uint16_t>(i));
   if(status==hf::DMPP06SecondLevel::Insert::inserted){auto got=h.lookup(key);
    require(got && got->payload==i,"reinsert missing");oracle[key]={got->stable_hashcode,static_cast<std::uint16_t>(i)};++checks;}
   else require(status==hf::DMPP06SecondLevel::Insert::overflow,"unexpected duplication");
  }
  require(h.verify(),"post-erase invariant");++checks;
  for(const auto&[k,v]:oracle){auto got=h.lookup(k);
   require(got && got->payload==v.second && got->stable_hashcode==v.first,"oracle mismatch");++checks;}
  require(h.size()==oracle.size(),"size mismatch");++checks;
 }
 try{hf::DMPP06SecondLevel x(8,4,4,3,1,1);throw std::runtime_error("invalid geometry accepted");}
 catch(const std::invalid_argument&){++checks;}
 std::cout<<"DMPP06 second-level: "<<checks<<" checks, "<<inserted<<" inserted, "<<deleted<<" erased, "<<overflow<<" overflow\n";
}
