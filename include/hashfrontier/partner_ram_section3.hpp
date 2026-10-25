#pragma once
// STOC 2025 Bender/Kuszmaul/Zhou, Section 3.1, Algorithms 1--3 ONLY.
// Isolated partner-pair encoding primitive. External logical-address oracle is
// deliberately NOT encoded using the RAM, so this is NOT a complete table.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace hf {
class PartnerRAMSection3 {
public:
    struct Stats {
        std::uint64_t writes=0, reads=0, swaps=0, sampled_slots=0,
                      failed_writes=0, changed_words=0;
    };
private:
    std::size_t bins_, B_, half_, slots_;
    std::uint64_t rng_;
    std::vector<std::uint64_t> physical_; // Exactly n keys; no spare slot or marker.
    std::vector<std::uint64_t> masks_;    // External random masks; NOT yet encoded.
    std::unordered_map<std::uint64_t,std::size_t> logical_slot_; // External retrieval oracle.
    std::size_t populated_words_=0;
    mutable Stats stats_{};
    static std::uint64_t mix(std::uint64_t x) {
        x+=0x9e3779b97f4a7c15ULL;
        x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;
        x=(x^(x>>27))*0x94d049bb133111ebULL;
        return x^(x>>31);
    }
    std::uint64_t rand64(){rng_=mix(rng_);return rng_;}
    std::size_t index_slot(std::size_t word) const {
        const std::size_t group=word/(B_/2);
        return group*B_ + word%(B_/2);
    }
    std::size_t original_slot(std::uint64_t x) const {return logical_slot_.at(x);}
    std::size_t original_bin(std::uint64_t x) const {return original_slot(x)/B_;}
    bool is_self_loop(std::size_t p) const {return original_slot(physical_[p])==p;}
    std::size_t partner_bin(std::uint64_t encoded) const {return half_+static_cast<std::size_t>(encoded);}
    std::optional<std::size_t> select_self_loop(std::size_t bin) {
        for(std::size_t trial=0;trial<2*B_;++trial){
            const std::size_t slot=bin*B_+(rand64()%B_);
            ++stats_.sampled_slots;
            if(is_self_loop(slot))return slot;
        }
        // Finite-test safeguard; not claimed to be the paper's O(1) proof.
        for(std::size_t t=0;t<B_;++t){
            const std::size_t slot=bin*B_+t;
            ++stats_.sampled_slots;
            if(is_self_loop(slot))return slot;
        }
        return std::nullopt;
    }
public:
    PartnerRAMSection3(std::size_t bins,std::size_t bin_size,std::uint64_t seed)
     :bins_(bins),B_(bin_size),half_(bins/2),slots_(bins*bin_size),rng_(mix(seed)),
      physical_(slots_),masks_(bins*bin_size/4){
        if(bins<4 || (bins&(bins-1)) || B_<4 || (B_&1) || slots_>10000000ULL)
            throw std::invalid_argument("bins must be power-of-two >=4; B even >=4; n<=1e7");
        logical_slot_.reserve(slots_*2);
        for(std::size_t i=0;i<slots_;++i){
            std::uint64_t key=rand64();
            while(!key || logical_slot_.count(key))key=rand64();
            physical_[i]=key;logical_slot_.emplace(key,i);
        }
        for(auto &m:masks_)m=rand64()%half_;
    }
    std::size_t size() const {return slots_;}
    std::size_t capacity() const {return slots_;}
    std::size_t num_words() const {return masks_.size();}
    std::size_t word_alphabet() const {return half_;}
    std::size_t encoded_words() const {return populated_words_;}
    Stats stats() const {return stats_;}
    void reset_stats(){stats_={};}
    // Algorithm 1: logical address of x is physical location of x's partner.
    std::size_t find_partner(std::uint64_t x) const {return original_slot(x);}
    // Section 3.2 physical-position recovery by composing partner locations.
    std::size_t locate(std::uint64_t key) const {
        const std::size_t j=find_partner(key);
        return find_partner(physical_[j]);
    }
    bool contains(std::uint64_t key) const {
        const auto it=logical_slot_.find(key);
        if(it==logical_slot_.end())return false;
        return physical_[locate(key)]==key;
    }
    std::optional<std::size_t> read(std::size_t word) const {
        if(word>=num_words())throw std::out_of_range("word");
        ++stats_.reads;
        // Algorithm 2: key physically in index slot discloses partner bin.
        const std::size_t h=original_bin(physical_[index_slot(word)]);
        if(h<half_)return std::nullopt;
        return (h-half_) ^ static_cast<std::size_t>(masks_[word]);
    }
    void uncouple(std::size_t word) {
        const std::size_t idx=index_slot(word);
        const auto other=original_slot(physical_[idx]);
        if(other!=idx){std::swap(physical_[idx],physical_[other]);++stats_.swaps;}
    }
    // Algorithm 3: erase old pair, form pair with a self-loop in specified bin.
    // A missing self-loop is an explicit failure; old value is restored.
    bool write(std::size_t word,std::optional<std::size_t> value) {
        if(word>=num_words())throw std::out_of_range("word");
        if(value && *value>=half_)throw std::out_of_range("value");
        ++stats_.writes;
        const auto old=read(word);
        if(old==value)return true;
        if(old)uncouple(word);
        if(value){
            const std::size_t desired=partner_bin((*value)^static_cast<std::size_t>(masks_[word]));
            auto other=select_self_loop(desired);
            if(!other){
                ++stats_.failed_writes;
                if(old){
                    // The original partner slot is now a self-loop, so restore exactly.
                    const std::size_t old_bin=partner_bin((*old)^static_cast<std::size_t>(masks_[word]));
                    bool recovered=false;
                    for(std::size_t t=old_bin*B_;t<(old_bin+1)*B_;++t){
                        if(is_self_loop(t)) {std::swap(physical_[index_slot(word)],physical_[t]);++stats_.swaps;recovered=true;break;}
                    }
                    if(!recovered)throw std::logic_error("rollback partner missing");
                }
                return false;
            }
            std::swap(physical_[index_slot(word)],physical_[*other]);++stats_.swaps;
        }
        if(!old && value)++populated_words_;
        if(old && !value)--populated_words_;
        ++stats_.changed_words;
        return true;
    }

    // STOC 2025 Algorithm 5: finite *restricted* batch staging. All reads
    // and candidate selection use the unchanged pre-update physical array.
    // Overlapping movements abort unchanged; no DMPP06 retrieval claims.
    struct BatchStats {
        std::uint64_t committed=0, rejected=0, planned_swaps=0, prestate_reads=0;
    };
private:
    BatchStats batches_{};
public:
    BatchStats batch_stats() const { return batches_; }
    std::size_t destination_bin(std::size_t word,std::size_t value) const {
        if(word>=num_words()||value>=half_)throw std::out_of_range("destination bin");
        return partner_bin(value^static_cast<std::size_t>(masks_[word]));
    }
    bool write_batch(const std::vector<std::pair<std::size_t,std::optional<std::size_t>>>& requests) {
        struct Plan {std::size_t index,old_partner,new_partner;bool had_old,has_new;};
        std::vector<Plan> plan;
        std::unordered_set<std::size_t> touched,seen;
        touched.reserve(requests.size()*4+1);
        seen.reserve(requests.size()*2+1);
        for(auto [word,value]:requests){
            if(word>=num_words())throw std::out_of_range("batch word");
            if(value && *value>=half_)throw std::out_of_range("batch value");
            if(!seen.insert(word).second)throw std::invalid_argument("duplicate batch word");
        }
        plan.reserve(requests.size());
        for(auto [word,value]:requests){
            const auto old=read(word);
            ++batches_.prestate_reads;
            if(old==value)continue;
            const auto index=index_slot(word);
            const auto old_slot=old?original_slot(physical_[index]):index;
            if(old && old_slot==index)throw std::logic_error("broken prior coupling");
            auto target=index;
            if(value){
                const auto wanted=destination_bin(word,*value);
                const auto start=rand64()%B_;
                bool found=false;
                for(std::size_t k=0;k<B_;++k){
                    const auto pos=wanted*B_+(start+k)%B_;
                    if(!touched.contains(pos) && pos!=index && pos!=old_slot && is_self_loop(pos)){
                        target=pos;found=true;break;
                    }
                }
                if(!found){++batches_.rejected;return false;}
            }
            if(touched.contains(index)||(old && touched.contains(old_slot))||
               (value && touched.contains(target))){
                ++batches_.rejected;return false;
            }
            touched.insert(index);
            if(old)touched.insert(old_slot);
            if(value)touched.insert(target);
            plan.push_back({index,old_slot,target,old.has_value(),value.has_value()});
        }
        // This is the commit boundary: no slot was moved during staging.
        for(const auto& p:plan){
            if(p.had_old){std::swap(physical_[p.index],physical_[p.old_partner]);++stats_.swaps;}
            if(p.has_new){std::swap(physical_[p.index],physical_[p.new_partner]);++stats_.swaps;}
            if(p.had_old&&!p.has_new)--populated_words_;
            if(!p.had_old&&p.has_new)++populated_words_;
            ++stats_.changed_words;
            batches_.planned_swaps+=p.had_old+p.has_new;
        }
        ++batches_.committed;
        return true;
    }
    bool verify(const std::vector<std::optional<std::size_t>>& expected) const {
        if(expected.size()!=num_words())return false;
        for(std::size_t i=0;i<num_words();++i)if(read(i)!=expected[i])return false;
        for(std::size_t p=0;p<slots_;++p) {
            const auto key=physical_[p];
            if(!contains(key) || locate(key)!=p)return false;
            const auto j=original_slot(key);
            if(j!=p && original_slot(physical_[j])!=p)return false;
        }
        std::unordered_set<std::uint64_t> once(physical_.begin(),physical_.end());
        return once.size()==slots_;
    }
    std::size_t physical_bytes() const {return physical_.capacity()*sizeof(std::uint64_t);}
    // DO NOT represent as paper's no-external-metadata space bound.
    std::size_t external_metadata_lower_bound_bytes() const {
        return masks_.capacity()*sizeof(std::uint64_t)+logical_slot_.size()*(sizeof(std::uint64_t)+sizeof(std::size_t));
    }
};
} // namespace hf
