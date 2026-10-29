#include "hashfrontier/dmpp06_grouped_quotient.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
using Clock=std::chrono::steady_clock;
int main(){
    std::cout<<"method,keys,bucket_bits,distribution,seed,collision_keys,over_threshold_keys,max_bucket_load,metadata_bytes,hash_ns_per_key,valid\n";
    volatile std::uint64_t guard=0;
    for(unsigned bits:{9u,10u,11u,12u,13u})for(unsigned n:{4096u,8192u,16384u})
    for(unsigned profile=0;profile<2;++profile)for(unsigned seed=1;seed<=5;++seed){
        hf::DMPP06ColumnQuotient16 base(bits,(0x8747ULL+seed)^0xd6e8feb86659fd93ULL);
        hf::DMPP06GroupedQuotient16 group(bits,0x8747ULL+seed);
        std::vector<std::uint16_t> xs(65536);
        std::iota(xs.begin(),xs.end(),0);
        if(profile){std::mt19937_64 rng(seed*881+bits*19+n);std::shuffle(xs.begin(),xs.end(),rng);}
        xs.resize(n);
        for(int method=0;method<2;++method){
            std::vector<std::uint32_t> buckets(1u<<bits,0);
            auto t0=Clock::now();
            std::uint64_t sum=0;
            for(auto x:xs){
                auto [b,q]=method?group.quotient(x):base.quotient(x);
                if((method?group.restore(b,q):base.restore(b,q))!=x)
                    throw std::runtime_error("roundtrip failure in benchmark");
                ++buckets[b];sum+=b+q;
            }
            auto t1=Clock::now();
            // Measure hot hashing separately from inverse and bucket allocation.
            std::uint64_t checksum=0;
            auto start=Clock::now();
            for(int pass=0;pass<8;++pass)for(auto x:xs){
                auto [b,q]=method?group.quotient(x):base.quotient(x);
                checksum+=b+q;
            }
            auto end=Clock::now();
            guard=guard+checksum+sum;
            std::uint64_t collisions=0,overflow=0;
            unsigned maxload=0;
            const auto threshold=(5*n+4*(1u<<bits)-1)/(4*(1u<<bits));
            for(auto x:buckets){
                if(x>=2)collisions+=x;
                if(x>threshold)overflow+=x-threshold;
                maxload=std::max(maxload,x);
            }
            if(collisions>n||overflow>n||maxload>n||t1<t0)throw std::logic_error("counter");
            const double ns=std::chrono::duration<double,std::nano>(end-start).count()/(8*n);
            std::cout<<(method?"transformation4":"transformation3")<<','<<n<<','<<bits<<','<<(profile?"uniform":"contiguous")<<','<<seed<<','
                     <<collisions<<','<<overflow<<','<<maxload<<','<<(method?group.metadata_bytes():base.metadata_bytes())<<','<<ns<<",1\n";
        }
    }
    if(guard==0)std::cerr<<"unexpected checksum\n";
}
