#include "hashfrontier/partner_ram_section3.hpp"
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>
#include <vector>
using clock_type=std::chrono::steady_clock;
static double nano(clock_type::time_point a,clock_type::time_point b){return std::chrono::duration<double,std::nano>(b-a).count();}
int main(int argc,char**argv){
    const std::size_t repeats=argc>1?std::strtoul(argv[1],nullptr,10):8;
    std::cout<<"component,bins,bin_size,slots,seed,write_attempts,successful_writes,failed_writes,read_ops,write_ns_per_attempt,read_ns_per_op,physical_bytes,external_metadata_lower_bound_bytes,encoded_words,valid\n";
    for(const auto bins:{32UL,64UL,128UL})for(const auto B:{64UL,128UL,256UL})
    for(std::size_t s=1;s<=repeats;++s){
        hf::PartnerRAMSection3 ram(bins,B,0x89ABCDEFULL+s);
        std::mt19937_64 rng(12313+s*31+bins*7+B);
        std::vector<std::optional<std::size_t>> expected(ram.num_words());
        const std::size_t writes=12000;
        std::size_t accepted=0;
        auto t0=clock_type::now();
        for(std::size_t z=0;z<writes;++z){
            const std::size_t idx=rng()%ram.num_words();
            auto value=(rng()%4==0)?std::nullopt:std::optional<std::size_t>(rng()%ram.word_alphabet());
            if(ram.write(idx,value)){expected[idx]=value;++accepted;}
        }
        auto t1=clock_type::now();
        const std::size_t reads=120000;
        std::uint64_t checksum=0;
        auto t2=clock_type::now();
        for(std::size_t z=0;z<reads;++z){
            const auto idx=rng()%ram.num_words();
            const auto x=ram.read(idx);
            if(x!=expected[idx])throw std::logic_error("wrong read");
            checksum+=x?(*x+1):0;
        }
        auto t3=clock_type::now();
        if(!ram.verify(expected))throw std::logic_error("permutation corrupted");
        std::cout<<"partner_ram_3.1,"<<bins<<','<<B<<','<<ram.size()<<','<<s<<','<<writes<<','<<accepted<<','<<(writes-accepted)<<','<<reads<<','<<nano(t0,t1)/writes<<','<<nano(t2,t3)/reads<<','<<ram.physical_bytes()<<','<<ram.external_metadata_lower_bound_bytes()<<','<<ram.encoded_words()<<",1\n";
        if(checksum==UINT64_MAX)std::cerr<<checksum;
    }
}
