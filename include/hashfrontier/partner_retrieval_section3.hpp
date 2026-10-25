#pragma once
// STOC 2025 Algorithms 4-5 conditional subroutine only, NOT the full table.
// The key -> RAM-word index is EXTERNAL and DMPP06 is not implemented.
#include "hashfrontier/partner_ram_section3.hpp"
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>
namespace hf {
class PartnerRetrievalSection3 {
    PartnerRAMSection3 ram_;
    std::unordered_map<std::uint64_t,std::size_t> external_index_;
public:
    PartnerRetrievalSection3(std::size_t bins,std::size_t B,std::uint64_t seed,
                             const std::vector<std::uint64_t>& keys):ram_(bins,B,seed){
        if(keys.size()>ram_.num_words())throw std::invalid_argument("too many keys");
        for(std::size_t i=0;i<keys.size();++i)
            if(!keys[i]||!external_index_.emplace(keys[i],i).second)
                throw std::invalid_argument("duplicate/zero key");
    }
    std::size_t size()const{return external_index_.size();}
    std::size_t capacity()const{return ram_.num_words();}
    std::optional<std::size_t> query_retrieval(std::uint64_t k)const {
        auto i=external_index_.find(k);
        if(i==external_index_.end())return {};
        return ram_.read(i->second);
    }
    bool update_retrieval(const std::vector<std::pair<std::uint64_t,std::optional<std::size_t>>>& updates){
        std::vector<std::pair<std::size_t,std::optional<std::size_t>>> pending;
        pending.reserve(updates.size());
        for(const auto&[k,v]:updates){
            auto i=external_index_.find(k);
            if(i==external_index_.end())throw std::out_of_range("unknown key");
            pending.emplace_back(i->second,v);
        }
        return ram_.write_batch(pending);
    }
    bool verify(const std::unordered_map<std::uint64_t,std::optional<std::size_t>>& expected)const{
        if(expected.size()!=size())return false;
        std::vector<std::optional<std::size_t>> values(capacity());
        for(auto[k,i]:external_index_){
            auto it=expected.find(k);
            if(it==expected.end()||query_retrieval(k)!=it->second)return false;
            values[i]=it->second;
        }
        return ram_.verify(values);
    }
    std::size_t external_metadata_min_bytes()const {
        return ram_.external_metadata_lower_bound_bytes()+
               external_index_.size()*(sizeof(std::uint64_t)+sizeof(std::size_t));
    }
    std::size_t physical_bytes()const{return ram_.physical_bytes();}
    PartnerRAMSection3::BatchStats batch_stats()const{return ram_.batch_stats();}
};
}
