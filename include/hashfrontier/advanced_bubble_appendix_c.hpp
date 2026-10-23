#pragma once
// SODA 2025, Appendix C: deletion via physical tombstones, ordinary eviction
// participation, reactivation of a deleted key, scheduled full rebuild at
// augmented-load threshold. This wraps the independently implemented §5.2
// insertion/query algorithm; finite parameters DO NOT establish Corollary 3.1.
#include "hashfrontier/advanced_bubble_up.hpp"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace hf {
class AdvancedBubbleAppendixC {
    std::size_t capacity_, choices_, core_, max_chain_, max_augmented_;
    double alpha_;
    std::uint64_t seed_;
    std::uint64_t rebuilds_=0, failed_rebuilds_=0;
    bool appendix_b_choice_prime_=false;
    AdvancedBubbleUp table_;

    bool rebuild() {
        const auto keys=table_.live_keys_for_rebuild();
        // Full rehash and tombstone clearance. Rebuild is transactional: no
        // partially rebuilt table can replace the valid live dictionary.
        for(std::uint64_t attempt=1;attempt<=4;attempt++) {
            AdvancedBubbleUp candidate(capacity_,choices_,core_,alpha_,
                  seed_+0x9e3779b97f4a7c15ULL*(rebuilds_+attempt),max_chain_,appendix_b_choice_prime_);
            bool ok=true;
            for(auto key:keys) if(!candidate.insert(key)) {ok=false;break;}
            if(ok && candidate.valid() && candidate.live_size()==keys.size()) {
                table_=std::move(candidate);++rebuilds_;return true;
            }
        }
        ++failed_rebuilds_;
        return false;
    }
public:
    AdvancedBubbleAppendixC(std::size_t capacity,std::size_t choices,
          std::size_t core,double alpha,std::uint64_t seed,
          double max_live_load,double max_augmented_load,std::size_t max_chain=2048,
          bool appendix_b_choice_prime=false)
        : capacity_(capacity),choices_(choices),core_(core),max_chain_(max_chain),
          max_augmented_(static_cast<std::size_t>(std::floor(capacity*max_augmented_load))),
          alpha_(alpha),seed_(seed),appendix_b_choice_prime_(appendix_b_choice_prime),
          table_(capacity,choices,core,alpha,seed,max_chain,appendix_b_choice_prime) {
        if(!(max_live_load>0 && max_live_load<max_augmented_load && max_augmented_load<1) ||
            max_augmented_<=static_cast<std::size_t>(std::ceil(capacity*max_live_load)))
            throw std::invalid_argument("Appendix C requires augmented-load headroom");
    }
    std::size_t size() const {return table_.live_size();}
    std::size_t physical_size() const {return table_.size();}
    std::size_t tombstones() const {return table_.tombstone_count();}
    std::size_t capacity() const {return capacity_;}
    std::size_t memory_bytes() const {return table_.memory_bytes();}
    std::uint64_t rebuilds() const {return rebuilds_;}
    std::uint64_t failed_rebuilds() const {return failed_rebuilds_;}
    bool contains(std::uint64_t key) const {return table_.contains(key);}
    bool erase(std::uint64_t key) {return table_.mark_deleted(key);}
    bool insert(std::uint64_t key) {
        if(table_.augmented_contains(key)) return table_.insert(key);
        if(table_.size()+1>max_augmented_ && !rebuild()) return false;
        if(table_.insert(key)) return true;
        // Appendix C explicitly permits a rebuild on rare insertion failure.
        if(!rebuild()) return false;
        return table_.insert(key);
    }
    bool valid() const {return table_.valid() && table_.live_size()<=table_.size();}
};
}
