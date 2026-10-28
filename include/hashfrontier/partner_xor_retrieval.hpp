#pragma once
// Experimental, finite-size XOR retrieval layout encoded IN PartnerRAMSection3.
// NOT the dynamic succinct DMPP06 retrieval structure used in STOC 2025.
// Updates use whole-structure peeling + RAM rewrites, not O(1) worst-case time.
// The active key/value registry is EXTERNAL and retained for rebuilding.
#include "hashfrontier/partner_ram_section3.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace hf {
class PartnerXorRetrieval {
public:
    struct Stats {
        std::uint64_t successful_rebuilds=0, failed_rebuilds=0;
        std::uint64_t peel_attempts=0, placement_attempts=0;
        std::uint64_t ram_batch_swaps=0, reconstructed_words=0;
    };
private:
    struct Edge {
        std::array<std::size_t, 3> indices{};
        std::size_t value=0;
    };
    PartnerRAMSection3 ram_;
    // External state is intentional and part of the reported space overhead.
    std::unordered_map<std::uint64_t,std::size_t> active_;
    std::uint64_t hash_seed_;
    std::uint64_t layout_sequence_=0;
    Stats stats_{};

    static std::uint64_t mix(std::uint64_t x) {
        x+=0x9e3779b97f4a7c15ULL;
        x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;
        x=(x^(x>>27))*0x94d049bb133111ebULL;
        return x^(x>>31);
    }
    static std::array<std::size_t,3> positions(std::uint64_t key,std::size_t size,std::uint64_t seed) {
        if(size<3)throw std::invalid_argument("XOR retrieval needs >=3 RAM words");
        std::array<std::size_t,3> out{};
        for(std::size_t j=0;j<3;++j){
            auto pos=static_cast<std::size_t>(mix(key ^ mix(seed+j*0xd6e8feb86659fd93ULL))%size);
            for(std::size_t trial=0;trial<size;++trial){
                bool duplicate=false;
                for(std::size_t k=0;k<j;++k)duplicate|=out[k]==pos;
                if(!duplicate)break;
                pos=(pos+1)%size;
            }
            out[j]=pos;
        }
        return out;
    }
    // Standard 3-uniform XOR retrieval peeling, NOT DMPP06 dynamic retrieval.
    static bool build_words(const std::unordered_map<std::uint64_t,std::size_t>& pairs,
                            std::size_t word_count,std::uint64_t seed,
                            std::vector<std::size_t>& cells){
        std::vector<Edge> edges;
        edges.reserve(pairs.size());
        std::vector<std::vector<std::size_t>> incident(word_count);
        std::vector<std::size_t> degree(word_count,0);
        for(const auto& [key,value]:pairs){
            const auto indices=positions(key,word_count,seed);
            const auto ei=edges.size();
            edges.push_back({indices,value});
            for(auto j:indices){incident[j].push_back(ei);++degree[j];}
        }
        std::deque<std::size_t> leaf;
        for(std::size_t i=0;i<word_count;++i)if(degree[i]==1)leaf.push_back(i);
        std::vector<bool> removed(edges.size(),false);
        std::vector<std::pair<std::size_t,std::size_t>> peeled;
        peeled.reserve(edges.size());
        while(!leaf.empty()){
            const auto vertex=leaf.front();leaf.pop_front();
            if(degree[vertex]!=1)continue;
            std::size_t chosen=edges.size();
            for(auto e:incident[vertex])if(!removed[e]){chosen=e;break;}
            if(chosen==edges.size())throw std::logic_error("peeling degree inconsistency");
            removed[chosen]=true;
            peeled.push_back({chosen,vertex});
            for(auto j:edges[chosen].indices){
                if(degree[j]==0)throw std::logic_error("peeling underflow");
                --degree[j];
                if(degree[j]==1)leaf.push_back(j);
            }
        }
        if(peeled.size()!=edges.size())return false;
        cells.assign(word_count,0);
        for(auto it=peeled.rbegin();it!=peeled.rend();++it){
            const auto& e=edges[it->first];
            std::size_t combined=e.value;
            for(auto idx:e.indices)if(idx!=it->second)combined^=cells[idx];
            cells[it->second]=combined;
        }
        return true;
    }
    std::size_t cell(std::size_t word) const {return ram_.read(word).value_or(0);}
    bool stage(const std::unordered_map<std::uint64_t,std::size_t>& proposed){
        // A small occupancy regime is required for reliable peeling. This is
        // NOT a paper bound and does not apply to arbitrary-sized retrievals.
        if(proposed.size()>ram_.num_words()/5)return false;
        for(std::size_t hash_try=0;hash_try<32;++hash_try){
            const auto new_seed=mix(hash_seed_^mix(layout_sequence_+hash_try+1));
            std::vector<std::size_t> target;
            ++stats_.peel_attempts;
            if(!build_words(proposed,ram_.num_words(),new_seed,target))continue;
            for(std::size_t placement_try=0;placement_try<8;++placement_try){
                ++stats_.placement_attempts;
                auto staged=ram_;
                std::vector<std::pair<std::size_t,std::optional<std::size_t>>> updates;
                updates.reserve(target.size());
                // Deterministic cyclic schedule varies placement sequence.
                for(std::size_t j=0;j<target.size();++j){
                    const auto idx=(j+placement_try*103)%target.size();
                    if(cell(idx)==target[idx])continue;
                    updates.emplace_back(idx,target[idx] ? std::optional<std::size_t>(target[idx]):std::nullopt);
                }
                if(!staged.write_batch_reuse(updates))continue;
                bool correct=true;
                for(std::size_t i=0;i<target.size();++i)
                    if(staged.read(i).value_or(0)!=target[i]){correct=false;break;}
                if(!correct)throw std::logic_error("encoded partner RAM failed independent readback");
                stats_.ram_batch_swaps+=staged.batch_stats().planned_swaps-ram_.batch_stats().planned_swaps;
                stats_.reconstructed_words+=updates.size();
                ram_=std::move(staged);
                active_=proposed;
                hash_seed_=new_seed;
                ++layout_sequence_;
                ++stats_.successful_rebuilds;
                return true;
            }
        }
        ++stats_.failed_rebuilds;
        return false;
    }
public:
    PartnerXorRetrieval(std::size_t bins,std::size_t bin_size,std::uint64_t seed)
      :ram_(bins,bin_size,seed),hash_seed_(mix(seed^0xa5a5beefface1234ULL)){
        if((bins/2)<2 || (bins/2 & ((bins/2)-1))!=0)
            throw std::invalid_argument("value alphabet must be a power of two");
    }
    std::size_t size()const{return active_.size();}
    std::size_t word_count()const{return ram_.num_words();}
    std::size_t alphabet()const{return ram_.word_alphabet();}
    Stats stats()const{return stats_;}
    // Retrieval does NOT prove membership; absent-key outputs are arbitrary.
    std::size_t retrieve(std::uint64_t key) const {
        std::size_t v=0;
        for(auto i:positions(key,word_count(),hash_seed_))v^=cell(i);
        return v;
    }
    bool upsert(std::uint64_t key,std::size_t value){
        if(!key || value>=alphabet())throw std::invalid_argument("key/value outside retrieval domain");
        if(auto it=active_.find(key);it!=active_.end() && it->second==value)return true;
        auto proposed=active_;
        proposed[key]=value;
        return stage(proposed);
    }
    bool erase(std::uint64_t key){
        if(!active_.count(key))return false;
        auto proposed=active_;
        proposed.erase(key);
        return stage(proposed);
    }
    bool verify()const{
        for(auto [key,value]:active_)if(retrieve(key)!=value)return false;
        return true;
    }
    std::size_t encoded_array_bytes()const{return ram_.physical_bytes();}
    std::size_t external_lower_bound_bytes()const{
        return ram_.external_metadata_lower_bound_bytes()+
            active_.size()*(sizeof(std::uint64_t)+sizeof(std::size_t));
    }
};
} // namespace hf
