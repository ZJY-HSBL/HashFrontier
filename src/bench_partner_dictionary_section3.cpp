// STOC 2025 Partner Hashing §3.2 paper Algorithms 6--8 -- oracle-backed pilot.
// This is an engineering timing diagnostic, NOT an implementation of implicit
// retrieval Algorithms 4/5, and NOT the paper's metadata-free complexity result.
#include "hashfrontier/partner_dictionary_section3.hpp"
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>
#include <unordered_set>
#include <vector>
using H=hf::PartnerDictionarySection3;
using Clock=std::chrono::steady_clock;
static double ns(Clock::time_point a,Clock::time_point b){return std::chrono::duration<double,std::nano>(b-a).count();}
int main(int argc,char**argv){
 const int seeds=argc>1?std::atoi(argv[1]):5;
 if(seeds<=0||seeds>30)throw std::invalid_argument("seed count must be 1..30");
 std::cout<<"component,bins,bin_size,capacity,initial_keys,seed,"
             "hit_ns_op,miss_ns_op,ram_write_ns_attempt,delete_ns_attempt,insert_ns_attempt,"
             "write_attempts,write_successes,delete_attempts,delete_successes,insert_successes,"
             "physical_bytes,external_metadata_min_bytes,checksum,valid\n";
 for(auto bins:{8UL,16UL,32UL,64UL})for(auto B:{16UL,32UL,64UL})
 for(int seed=1;seed<=seeds;++seed){
   std::size_t needed=3*B/4;
   auto hasher=[&](std::uint64_t key){return H::hash_mix(key^H::hash_mix((std::uint64_t)seed^0x9ae16a3b2f90404fULL))%bins;};
   std::vector<std::vector<std::uint64_t>> pool(bins);
   std::size_t total=0;
   for(std::uint64_t key=1;total<bins*(needed+B+300);++key){
     auto b=hasher(key);
     if(pool[b].size()<needed+B+300){pool[b].push_back(key);++total;}
   }
   std::vector<std::uint64_t> init;
   std::vector<std::vector<std::uint64_t>> live(bins);
   for(auto b=0UL;b<bins;++b)for(auto i=0UL;i<needed;++i){init.push_back(pool[b][i]);live[b].push_back(pool[b][i]);}
   H h(bins,B,seed,init);
   std::vector<std::optional<std::size_t>> ram(h.words());
   std::mt19937_64 rng(seed*371+bins*19+B);
   // Pre-generate lookup requests; no input RNG in timed loops.
   std::vector<std::uint64_t> hits, misses;
   for(int i=0;i<2000;++i){hits.push_back(init[rng()%init.size()]);misses.push_back(UINT64_C(0xf00d000000000001)+(std::uint64_t)i);}
   std::uint64_t checksum=0;
   auto t0=Clock::now();
   for(auto key:hits)checksum+=h.contains(key);
   auto t1=Clock::now();
   for(auto key:misses)checksum+=h.contains(key);
   auto t2=Clock::now();
   if(checksum!=hits.size())throw std::runtime_error("membership oracle violated");
   const std::size_t write_attempts=400;
   std::size_t write_success=0;
   auto t3=Clock::now();
   for(std::size_t i=0;i<write_attempts;++i){
     const auto w=rng()%h.words();
     auto v=(rng()%5==0)?std::nullopt:std::optional<std::size_t>(rng()%h.alphabet());
     if(h.write_ram(w,v)){ram[w]=v;++write_success;}
   }
   auto t4=Clock::now();
   if(!h.verify(ram))throw std::runtime_error("RAM state corrupted by writes");
   // Experimental insert/delete on paper §3.2's occupied-prefix convention.
   // Every delete is conditionally compensated by an insertion in the same bin.
   std::size_t dels=0, ins=0;
   std::vector<std::size_t> fresh(bins,needed);
   const std::size_t mutations=240;
   std::uint64_t del_ns=0,ins_ns=0;
   for(std::size_t i=0;i<mutations;++i){
     auto bin=rng()%bins;
     auto idx=rng()%live[bin].size();
     const auto victim=live[bin][idx];
     auto start=Clock::now();
     bool ok=h.erase_key(victim);
     auto stop=Clock::now();
     del_ns+=static_cast<std::uint64_t>(ns(start,stop));
     if(!ok)continue;
     ++dels;
     live[bin][idx]=live[bin].back();live[bin].pop_back();
     const auto replacement=pool[bin][fresh[bin]++];
     start=Clock::now();
     const bool inserted=h.insert_key(replacement);
     stop=Clock::now();
     ins_ns+=static_cast<std::uint64_t>(ns(start,stop));
     if(!inserted)throw std::runtime_error("replacement insert failed after successful delete");
     ++ins;
     live[bin].push_back(replacement);
   }
   if(!h.verify(ram))throw std::runtime_error("final retrieval/coupling/packed-bin invariant");
   if(h.size()!=init.size())throw std::runtime_error("initial/final size mismatch");
   for(const auto &bin:live)for(auto key:bin){if(!h.contains(key))throw std::runtime_error("missing live key");}
   std::cout<<"partner_section3_2_oracle,"<<bins<<','<<B<<','<<h.capacity()<<','<<init.size()<<','<<seed<<','
            <<ns(t0,t1)/hits.size()<<','<<ns(t1,t2)/misses.size()<<','
            <<ns(t3,t4)/write_attempts<<','<<double(del_ns)/mutations<<','
            <<(dels?double(ins_ns)/dels:0.0)<<','
            <<write_attempts<<','<<write_success<<','<<mutations<<','<<dels<<','<<ins<<','
            <<h.physical_bytes()<<','<<h.external_metadata_min_bytes()<<','<<checksum<<",1\n";
 }
}
