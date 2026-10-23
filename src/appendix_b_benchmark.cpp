// Independent SODA 2025 Advanced §5.2 idealized-choice and Appendix B choice'
// finite engineering comparison. Same randomized key/absence arrays per pair.
#include "hashfrontier/advanced_bubble_up.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_set>
#include <vector>
using Clock=std::chrono::steady_clock;
static double elapsed(Clock::time_point a,Clock::time_point b){return std::chrono::duration<double,std::nano>(b-a).count();}
static std::vector<std::uint64_t> random_keys(std::size_t count, std::uint64_t seed){
    std::mt19937_64 rng(seed);
    std::vector<std::uint64_t> keys;keys.reserve(count);
    std::unordered_set<std::uint64_t> seen;
    while(keys.size()<count){auto key=rng();if(seen.insert(key).second)keys.push_back(key);}
    return keys;
}
int main(int argc,char**argv){
    const int seeds=argc>1?std::stoi(argv[1]):10;
    if(seeds<1||seeds>1000)throw std::invalid_argument("1 <= seeds <= 1000");
    std::uint64_t checksum=0;
    std::cout<<"algorithm,capacity,load_pct,seed,requested,inserted,complete,correct,"
               "build_ns_per_key,hit_ns_per_op,miss_ns_per_op,bytes_per_key,"
               "active_d,phases,probes,noncore_probes,evictions,core_moves,"
               "choice_prime_evaluations,core_collision_redirects,failures\n";
    std::cout<<std::fixed<<std::setprecision(5);
    for(auto capacity:{4096ULL,16384ULL})for(int load:{70,85,95}){
        const auto target=std::size_t(capacity*static_cast<unsigned>(load)/100);
        for(int seed=101;seed<101+seeds;++seed){
            const auto positives=random_keys(target,std::uint64_t(seed)*0x9e3779b97f4a7c15ULL+capacity);
            const auto absent=random_keys(2048,std::uint64_t(seed)*0xb5ad4eceda1ce2a9ULL+capacity);
            const std::unordered_set<std::uint64_t> all(positives.begin(),positives.end());
            for(const bool appendix_b:{false,true}){
                hf::AdvancedBubbleUp ht(capacity,9,3,.95,seed,4096,appendix_b);
                std::size_t inserted=0;
                auto start=Clock::now();
                for(auto k:positives){if(!ht.insert(k))break;++inserted;}
                auto end=Clock::now();
                const auto build_ns=elapsed(start,end)/std::max<std::size_t>(inserted,1);
                bool correct=ht.valid()&&ht.size()==inserted;
                start=Clock::now();
                for(std::size_t i=0;i<inserted;i++)if(!ht.contains(positives[i]))correct=false;
                end=Clock::now();
                const auto hit_ns=elapsed(start,end)/std::max<std::size_t>(inserted,1);
                start=Clock::now();
                for(auto k:absent)if(ht.contains(k)!=(all.count(k)>0 &&
                        std::find(positives.begin(),positives.begin()+inserted,k)!=positives.begin()+inserted))correct=false;
                end=Clock::now();
                const auto miss_ns=elapsed(start,end)/absent.size();
                if(!correct)throw std::runtime_error("oracle mismatch or broken cuckoo placement");
                const auto stat=ht.stats();
                std::cout<<(appendix_b?"advanced_appendix_b_choice_prime":"advanced_s52_stored_choice")<<','
                    <<capacity<<','<<load<<','<<seed<<','<<target<<','<<inserted<<','
                    <<(inserted==target?1:0)<<','<<(correct?1:0)<<','
                    <<build_ns<<','<<hit_ns<<','<<miss_ns<<','
                    <<static_cast<double>(ht.memory_bytes())/std::max<std::size_t>(inserted,1)<<','
                    <<ht.active_choices()<<','<<ht.phases()<<','<<stat.probes<<','<<stat.noncore_probes<<','
                    <<stat.evictions<<','<<stat.core_moves<<','<<stat.choice_prime_evaluations<<','
                    <<stat.core_collision_redirects<<','<<stat.failures<<'\n';
                checksum^=inserted+(stat.evictions<<1)+(stat.probes<<2);
            }
        }
    }
    std::cerr<<"audit_checksum="<<checksum<<'\n';
}
