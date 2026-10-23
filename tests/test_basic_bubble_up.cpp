#include "hashfrontier/basic_bubble_up.hpp"
#include <cstdint>
#include <unordered_set>
#include <random>
#include <iostream>
#include <stdexcept>
int main(){
 std::size_t ops=0;
 for(int pct:{50,70,85,95})for(std::uint64_t seed=1;seed<=12;seed++){
   constexpr std::size_t cap=4096;
   hf::BasicBubbleUp h(cap,pct,seed);std::unordered_set<std::uint64_t> ref;
   std::mt19937_64 gen(seed*12345+pct);
   while(ref.size()<cap*static_cast<std::size_t>(pct)/100){auto k=gen();if(ref.insert(k).second && !h.insert(k))throw std::runtime_error("insert failed");ops++;}
   for(auto k:ref){if(!h.contains(k))throw std::runtime_error("lost build key");ops++;}
   for(int c=0;c<150;c++){
     auto it=ref.begin();std::advance(it,gen()%ref.size());auto old=*it;
     if(!h.erase(old))throw std::runtime_error("delete failed");
     ref.erase(it);
     auto k=gen();while(ref.contains(k))k=gen();
     if(!h.insert(k))throw std::runtime_error("churn insert failed");
     ref.insert(k);
     if(h.contains(old)||!h.contains(k)||h.size()!=ref.size())throw std::runtime_error("membership mismatch");
     ops+=4;
   }
   for(auto k:ref) {if(!h.contains(k))throw std::runtime_error("lost after churn");ops++;}
 }
 std::cout<<"PASS basic bubble-up section 4 insert/delete/query randomized "<<ops<<" assertions/operations\n";
}
