#include "hashfrontier/dmpp06_column_quotient.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>
using Clock=std::chrono::steady_clock;
int main(int argc,char**argv){
 unsigned seeds=argc>1?static_cast<unsigned>(std::stoul(argv[1])):10;
 std::cout<<"family,bucket_bits,buckets,keys,distribution,seed,colliding_keys,max_bucket,forward_ns_per_op,inverse_ns_per_op,metadata_bytes,correct\n";
 std::uint64_t sink=0;
 for(unsigned bits:{10u,11u,12u})for(unsigned n:{512u,1024u,2048u}){
   if(n>(1u<<bits))continue; // Section 2.2: case b >= n.
   for(const char* shape:{"contiguous","uniform"})
    for(unsigned seed=1;seed<=seeds;++seed){
     hf::DMPP06ColumnQuotient16 h(bits,seed*192734ULL+0xace5);
     std::vector<std::uint16_t> keys(65536);
     std::iota(keys.begin(),keys.end(),0);
     if(shape[0]=='u'){
       std::mt19937 rng(seed*929+bits*503+n);
       std::shuffle(keys.begin(),keys.end(),rng);
     }
     keys.resize(n);
     std::vector<unsigned> hist(h.bucket_count());
     std::vector<std::pair<std::uint16_t,std::uint16_t>> encoded;
     encoded.reserve(n);
     auto a=Clock::now();
     for(auto k:keys){
       auto [b,q]=h.quotient(k);encoded.emplace_back(b,q);
       ++hist[b];sink+=b;
     }
     auto b=Clock::now();
     for(std::size_t i=0;i<keys.size();++i){
       if(h.restore(encoded[i].first,encoded[i].second)!=keys[i])return 3;
       sink+=keys[i];
     }
     auto c=Clock::now();
     unsigned collision_keys=0,max_bucket=0;
     for(auto k:hist){
       if(k>=2)collision_keys+=k;
       max_bucket=std::max(max_bucket,k);
     }
     std::cout<<"row_shift_column_affine,"<<bits<<','<<h.bucket_count()<<','<<n<<','<<shape<<','<<seed<<','<<collision_keys<<','<<max_bucket<<','
       <<std::chrono::duration<double,std::nano>(b-a).count()/n<<','
       <<std::chrono::duration<double,std::nano>(c-b).count()/n<<','
       <<h.metadata_bytes()<<",1\n";
   }
 }
 if(sink==0x123456789ULL)std::cerr<<sink;
}
