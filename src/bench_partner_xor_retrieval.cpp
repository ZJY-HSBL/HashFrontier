#include "hashfrontier/partner_xor_retrieval.hpp"
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>
using Clock=std::chrono::steady_clock;
static double ns(Clock::time_point a,Clock::time_point b){return std::chrono::duration<double,std::nano>(b-a).count();}
int main(int argc,char**argv){
 const int seeds=argc>1?std::atoi(argv[1]):3;
 if(seeds<1||seeds>30)throw std::invalid_argument("seeds must be 1..30");
 std::cout<<"bins,bin_size,ram_words,seed,keys_built,build_ns_per_key,hit_ns_per_query,update_ns_per_op,delete_ns_per_op,encoded_array_bytes,external_metadata_lower_bound_bytes,successful_rebuilds,peel_attempts,placement_attempts,valid\n";
 for(auto bins:{64UL,128UL,256UL})for(auto B:{32UL,64UL})for(int s=1;s<=seeds;++s){
   hf::PartnerXorRetrieval t(bins,B,0x1234+s);
   std::unordered_map<std::uint64_t,std::size_t> oracle;
   const auto n=std::min<std::size_t>(120,t.word_count()/10);
   std::mt19937_64 rng(s*19937+bins+B);
   std::vector<std::uint64_t> keys;
   keys.reserve(n);
   auto t0=Clock::now();
   for(std::size_t i=0;i<n;++i){
      const auto key=1000000+i*17;
      const auto value=std::size_t(rng()%t.alphabet());
      if(!t.upsert(key,value))throw std::runtime_error("failed build");
      oracle[key]=value;keys.push_back(key);
   }
   auto t1=Clock::now();
   const int reads=20000;
   std::uint64_t checksum=0;
   auto t2=Clock::now();
   for(int i=0;i<reads;++i){
      const auto k=keys[rng()%keys.size()];
      const auto v=t.retrieve(k);
      if(v!=oracle.at(k))throw std::runtime_error("bad retrieval");
      checksum+=v;
   }
   auto t3=Clock::now();
   const auto updates=std::min<std::size_t>(40,n);
   auto t4=Clock::now();
   for(std::size_t i=0;i<updates;++i){
      const auto key=keys[i];
      const auto value=std::size_t(rng()%t.alphabet());
      if(!t.upsert(key,value))throw std::runtime_error("failed update");
      oracle[key]=value;
   }
   auto t5=Clock::now();
   const auto deletions=std::min<std::size_t>(20,n);
   auto t6=Clock::now();
   for(std::size_t i=0;i<deletions;++i){
     const auto key=keys.back();keys.pop_back();
     if(!t.erase(key))throw std::runtime_error("failed delete");
     oracle.erase(key);
   }
   auto t7=Clock::now();
   if(!t.verify()||t.size()!=oracle.size())throw std::runtime_error("final invalid");
   const auto st=t.stats();
   std::cout<<bins<<','<<B<<','<<t.word_count()<<','<<s<<','<<n<<','
     <<ns(t0,t1)/n<<','<<ns(t2,t3)/reads<<','<<ns(t4,t5)/updates<<','
     <<ns(t6,t7)/deletions<<','<<t.encoded_array_bytes()<<','
     <<t.external_lower_bound_bytes()<<','<<st.successful_rebuilds<<','
     <<st.peel_attempts<<','<<st.placement_attempts<<",1\n";
   if(checksum==UINT64_MAX)std::cerr<<checksum;
 }
}
