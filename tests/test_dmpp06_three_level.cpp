#include "hashfrontier/dmpp06_three_level.hpp"
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
static void check(bool x){if(!x)throw std::runtime_error("three-tier differential mismatch");}
int main(){
 using Table=hf::DMPP06ThreeLevel16;
 std::uint64_t checks=0,ops=0,third=0,overflows=0,stability=0;
 for(unsigned b1:{4u,6u})for(unsigned b2:{4u,6u})for(unsigned fc:{2u,4u})
 for(unsigned tiles:{2u,4u})for(unsigned seed=1;seed<=4;++seed){
  Table t(b1,fc,b2,tiles,2,256,3,seed*47,5,seed*113,seed*2027);
  std::mt19937_64 rng(seed*11321+b1*137+b2*43+fc*7+tiles);
  struct Ref{std::uint16_t value;std::size_t code;unsigned level;};
  std::unordered_map<std::uint16_t,Ref> oracle;
  for(int step=0;step<1800;++step){
   auto k=static_cast<std::uint16_t>(rng()%1200);
   auto v=static_cast<std::uint16_t>(rng());
   auto action=rng()%5;
   if(action<2){
    auto s=t.insert(k,v);
    if(oracle.contains(k)){check(s==Table::Insert::already_present);++checks;}
    else if(s==Table::Insert::overflow){++overflows;check(!t.lookup(k));++checks;}
    else{
     auto x=t.lookup(k);check(bool(x));check(x->payload==v);
     auto level=s==Table::Insert::first?1u:s==Table::Insert::second?2u:3u;
     check(x->level==level);checks+=3;
     oracle.emplace(k,Ref{v,x->hashcode,x->level});
     if(level==3)++third;
    }
   }else if(action==2){
    bool ok=t.update(k,v);check(ok==oracle.contains(k));++checks;
    if(ok)oracle.at(k).value=v;
   }else if(action==3){
    bool ok=t.erase(k);check(ok==oracle.contains(k));++checks;
    if(ok)oracle.erase(k);
   }else{
    auto x=t.lookup(k);check(bool(x)==oracle.contains(k));++checks;
    if(x){auto ref=oracle.at(k);check(x->payload==ref.value&&x->hashcode==ref.code&&x->level==ref.level);++checks;}
   }
   ++ops;
   if(step%97==0){
    check(t.verify());check(t.size()==oracle.size());checks+=2;
    std::unordered_set<std::size_t> codes;
    for(auto [key,ref]:oracle){
     auto x=t.lookup(key);
     check(x&&x->payload==ref.value&&x->hashcode==ref.code&&x->level==ref.level);
     check(codes.insert(ref.code).second);checks+=2;++stability;
    }
   }
  }
  check(t.verify());++checks;
 }
 {Table t(4,1,4,1,1,2,3,11,5,13,7);unsigned n3=0,rejected=0;
  for(std::uint32_t k=0;k<65536;++k){
   auto s=t.insert(static_cast<std::uint16_t>(k),static_cast<std::uint16_t>(k));
   if(s==Table::Insert::third)++n3;
   if(s==Table::Insert::overflow)++rejected;
   if(rejected>5)break;
  }
  check(n3==2&&rejected>0);check(t.verify());checks+=2;
 }
 check(third>0);++checks;
 std::cout<<"DMPP06 three-tier checks="<<checks<<" ops="<<ops<<" third="<<third
 <<" overflows="<<overflows<<" stable_code_checks="<<stability<<"\n";
}
