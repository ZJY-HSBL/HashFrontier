#include "hashfrontier/dmpp06_first_level.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>
static void check(bool x){if(!x)throw std::runtime_error("DMPP06 first-level differential failure");}
int main(){
 std::uint64_t checks=0, inserts=0, erased=0, overflow=0;
 for(std::uint16_t a:{1u,3u,0x1234u,0xffffu})for(std::uint16_t b:{0u,0xace1u}){
    hf::DMPP06Affine16 perm(a,b);
    std::vector<bool> seen(65536,false);
    for(std::uint32_t i=0;i<65536;++i){
      auto v=perm.forward(static_cast<std::uint16_t>(i));
      check(!seen[v]);seen[v]=true;
      check(perm.inverse(v)==i);checks+=2;
      for(unsigned bits:{4u,8u,12u}){
       auto [bucket,q]=perm.quotient(static_cast<std::uint16_t>(i),bits);
       check(perm.restore(bucket,q,bits)==i);++checks;
      }
    }
 }
 try{hf::DMPP06Affine16 x(0,1);throw std::runtime_error("accepted zero coefficient");}catch(const std::invalid_argument&){++checks;}
 for(std::uint16_t seed=1;seed<=6;++seed)for(unsigned bits:{4u,6u,8u})for(std::size_t cap:{4UL,16UL}){
   std::mt19937 rng(seed*173+bits*71+cap);
   hf::DMPP06FirstLevel table(bits,cap,static_cast<std::uint16_t>(rng()|1u),static_cast<std::uint16_t>(rng()));
   std::unordered_map<std::uint16_t,std::pair<std::uint32_t,std::uint16_t>> oracle;
   for(int i=0;i<2000;++i){
     auto k=static_cast<std::uint16_t>(rng());
     if(i%5==0){bool ok=table.erase(k);check(ok==(oracle.erase(k)>0));erased+=ok;}
     else if(i%5==1){auto v=static_cast<std::uint16_t>(rng());bool ok=table.update(k,v);
       check(ok==oracle.contains(k));if(ok)oracle[k].second=v;}
     else {auto v=static_cast<std::uint16_t>(rng());bool ok=table.insert(k,v);
       if(ok){auto p=table.lookup(k);check(p.has_value()&&p->second==v);oracle.emplace(k,*p);++inserts;}
       else if(!oracle.contains(k))++overflow;
     }
     if(i%200==0){
       check(table.verify());++checks;
       for(auto [key,expected]:oracle){auto actual=table.lookup(key);check(actual&&*actual==expected);++checks;}
     }
   }
   check(table.verify());++checks;
   for(auto [key,v]:oracle){check(table.lookup(key)==v);++checks;}
 }
 // All codes in one bucket must be exhausted: preserve existing data on failed insert.
 {hf::DMPP06FirstLevel t(1,1,1,0);check(t.insert(0,123));auto before=t.lookup(0);
  check(!t.insert(1,234));check(t.lookup(0)==before);check(t.erase(0));check(t.insert(1,234));check(t.verify());checks+=6;}
 std::cout<<"DMPP06 source-specific finite first-level checks="<<checks<<" inserted="<<inserts<<" erased="<<erased<<" overflow="<<overflow<<" PASS\n";
}
