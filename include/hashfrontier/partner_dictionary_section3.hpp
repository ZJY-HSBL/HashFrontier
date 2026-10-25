#pragma once
// Bender, Kuszmaul, Zhou, STOC 2025, Section 3.2 Algorithms 6--8.
// CONDITIONAL reproduction with an external retrieval oracle. NOT Algorithm 4/5,
// NOT the implicit/no-metadata structure of Sections 4--5. Independent finite
// implementation of the pairing and packed-logical-bin update mechanism.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace hf {
class PartnerDictionarySection3 {
public:
    struct Stats {
        std::uint64_t queries=0, inserts=0, deletes=0, word_reads=0,
          word_writes=0, swaps=0, sampled=0, failed_writes=0,
          failed_deletions=0, decoupled_words=0;
    };
private:
    std::size_t bins_, B_, half_;
    std::uint64_t seed_, rng_;
    std::vector<std::uint64_t> A_;   // 0 is empty, otherwise *only* a key
    std::unordered_map<std::uint64_t,std::size_t> retrieval_; // EXTERNAL oracle!
    std::vector<std::size_t> counts_; // EXTERNAL metadata, not paper's RAM yet!
    std::vector<std::uint64_t> masks_; // EXTERNAL random words, not stored in A!
    mutable Stats stats_{};
    static std::uint64_t mix(std::uint64_t x) {
        x+=0x9e3779b97f4a7c15ULL;
        x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;
        x=(x^(x>>27))*0x94d049bb133111ebULL;
        return x^(x>>31);
    }
    std::uint64_t next_random(){rng_=mix(rng_);return rng_;}
    std::size_t index_slot(std::size_t word) const {
        return (word/(B_/2))*B_ + word%(B_/2);
    }
    std::size_t word_of_index_slot(std::size_t slot) const {
        return (slot/B_)*(B_/2)+slot%B_;
    }
    std::size_t logical_slot(std::uint64_t key) const {
        return hash_bin(key)*B_+retrieval_.at(key);
    }
    bool self_loop(std::size_t p) const {
        return A_[p] && logical_slot(A_[p])==p;
    }
    std::optional<std::size_t> find_self_loop(std::size_t bin) {
        for(std::size_t t=0;t<B_*2;++t) {
            std::size_t p=bin*B_+next_random()%B_;
            ++stats_.sampled;
            if(self_loop(p)) return p;
        }
        // Bounded finite-regression fallback, not part of asymptotic guarantees.
        for(std::size_t t=0;t<B_;++t){
            std::size_t p=bin*B_+t;
            ++stats_.sampled;
            if(self_loop(p))return p;
        }
        return {};
    }
    void uncouple_key(std::uint64_t key,
                      std::vector<std::pair<std::size_t,std::size_t>>& erased) {
        std::size_t original=logical_slot(key);
        std::size_t actual=locate_present(key);
        if(original==actual)return;
        const std::size_t a=original/B_, b=actual/B_;
        const bool original_index=a<half_ && (original%B_)<B_/2;
        const bool actual_index=b<half_ && (actual%B_)<B_/2;
        if(original_index==actual_index)throw std::logic_error("couple endpoints must be index and partner");
        const auto index=original_index?original:actual;
        const std::size_t w=word_of_index_slot(index);
        auto value=read_ram(w);
        if(!value)throw std::logic_error("decoupling unencoded index");
        erased.emplace_back(w,*value);
        std::swap(A_[original],A_[actual]);++stats_.swaps;
        ++stats_.decoupled_words;
        if(!self_loop(original)||!self_loop(actual))throw std::logic_error("decoupling invariant");
    }
public:
    static std::uint64_t hash_mix(std::uint64_t x){return mix(x);}
    PartnerDictionarySection3(std::size_t bins,std::size_t B,std::uint64_t seed,
                              const std::vector<std::uint64_t>& initial)
        :bins_(bins),B_(B),half_(bins/2),seed_(seed),rng_(mix(seed)),
         A_(bins*B,0),counts_(bins,0),masks_(bins*B/4) {
        if(bins<4 || bins&(bins-1) || B<8 || B%2 || bins>4096 || B>4096)
            throw std::invalid_argument("invalid (bins,B)");
        retrieval_.reserve(initial.size()*2+1);
        for(auto key:initial) {
            if(!key||retrieval_.contains(key))throw std::invalid_argument("zero or duplicate key");
            auto bin=hash_bin(key);
            if(counts_[bin]==B_)throw std::invalid_argument("initial bin overflow");
            const auto off=counts_[bin]++;
            A_[bin*B_+off]=key;
            retrieval_.emplace(key,off);
        }
        for(auto c:counts_)if(c<B_/2)throw std::invalid_argument("unoccupied index slot / bin underflow");
        for(auto &mask:masks_)mask=next_random()%half_;
    }
    std::size_t hash_bin(std::uint64_t key)const {
        return static_cast<std::size_t>(mix(key^mix(seed_^0x9ae16a3b2f90404fULL))%bins_);
    }
    std::size_t size()const{return retrieval_.size();}
    std::size_t capacity()const{return A_.size();}
    std::size_t words()const{return masks_.size();}
    std::size_t alphabet()const{return half_;}
    std::size_t bin_count(std::size_t b)const{return counts_.at(b);}
    Stats stats()const{return stats_;}
    void reset_stats(){stats_={};}
    std::size_t find_partner(std::uint64_t key)const {
        // Algorithm 1 uses arbitrary in-range offset for missing key.
        auto it=retrieval_.find(key);
        return hash_bin(key)*B_+(it==retrieval_.end()?0:it->second);
    }
    std::size_t locate_present(std::uint64_t key)const {
        const auto j=logical_slot(key);
        if(A_[j]==0)throw std::logic_error("present key logical slot empty");
        return logical_slot(A_[j]); // paper Section 3.2 / Algorithm 6
    }
    bool contains(std::uint64_t key) const {
        ++stats_.queries;
        if(!key)return false;
        auto s1=find_partner(key);
        if(A_[s1]==0)return false;
        const auto it=retrieval_.find(A_[s1]);
        if(it==retrieval_.end())throw std::logic_error("physical key missing retrieval entry");
        auto s2=hash_bin(A_[s1])*B_+it->second;
        return A_[s2]==key;
    }
    std::optional<std::size_t> read_ram(std::size_t w)const {
        if(w>=words())throw std::out_of_range("word");
        ++stats_.word_reads;
        auto p=index_slot(w);
        if(!A_[p])throw std::logic_error("empty index slot: section 3.1 precondition violated");
        auto bin=hash_bin(A_[p]);
        if(bin<half_)return {};
        return (bin-half_)^static_cast<std::size_t>(masks_[w]);
    }
    bool write_ram(std::size_t w,std::optional<std::size_t> value) {
        if(w>=words())throw std::out_of_range("word");
        if(value&&*value>=alphabet())throw std::out_of_range("RAM value");
        ++stats_.word_writes;
        auto old=read_ram(w);
        if(old==value)return true;
        const auto p=index_slot(w);
        std::size_t other=0;
        if(old) {
            other=logical_slot(A_[p]);
            std::swap(A_[p],A_[other]);++stats_.swaps;
        }
        if(value) {
            const auto bin=half_+((*value)^static_cast<std::size_t>(masks_[w]));
            auto candidate=find_self_loop(bin);
            if(!candidate){
                ++stats_.failed_writes;
                if(old){std::swap(A_[p],A_[other]);++stats_.swaps;}
                return false;
            }
            std::swap(A_[p],A_[*candidate]);++stats_.swaps;
        }
        return true;
    }
    bool insert_key(std::uint64_t key) {
        if(!key || retrieval_.contains(key))return false;
        auto bin=hash_bin(key);
        auto off=counts_[bin];
        if(off>=B_)return false; // failure event of Theorem 3.1
        if(A_[bin*B_+off]!=0)throw std::logic_error("packed logical bin violated");
        A_[bin*B_+off]=key;
        retrieval_.emplace(key,off);
        ++counts_[bin];++stats_.inserts;
        return true;
    }
    bool erase_key(std::uint64_t key) {
        if(!key||!retrieval_.contains(key))return false;
        const auto bin=hash_bin(key);
        if(counts_[bin]<=B_/2)return false; // keep every index slot occupied
        // Finite-test transactional rollback; deliberately NOT O(1) paper update.
        // Required because partner-bin write failures are possible at toy sizes.
        auto save_A=A_;
        auto save_map=retrieval_;
        auto save_counts=counts_;
        std::vector<std::pair<std::size_t,std::size_t>> erased;
        const auto tail=bin*B_+counts_[bin]-1;
        try {
            uncouple_key(key,erased);
            const auto y=A_[tail];
            if(y!=key)uncouple_key(y,erased);
            const auto target=logical_slot(key);
            const auto tail_key=A_[tail];
            if(!tail_key)throw std::logic_error("tail missing");
            if(tail!=target){
                A_[target]=tail_key;
                retrieval_.at(tail_key)=target%B_;
            }
            A_[tail]=0;
            retrieval_.erase(key);
            --counts_[bin];
            // Re-encode RAM words erased when endpoint pairs were split.
            for(auto [w,v]:erased)if(!write_ram(w,v)) {
                A_=std::move(save_A);retrieval_=std::move(save_map);counts_=std::move(save_counts);
                ++stats_.failed_deletions;
                return false;
            }
            ++stats_.deletes;
            return true;
        }catch(...){
            A_=std::move(save_A);retrieval_=std::move(save_map);counts_=std::move(save_counts);
            throw;
        }
    }
    bool verify(const std::vector<std::optional<std::size_t>>& ram)const{
        if(ram.size()!=words())return false;
        std::size_t occupied=0;
        std::unordered_set<std::uint64_t> keys;
        keys.reserve(size()*2);
        for(std::size_t b=0;b<bins_;++b)for(std::size_t i=0;i<B_;++i) {
            auto logical=b*B_+i;
            // logical slot occupied iff offset < bin count; physical occupancy same
            if(bool(A_[logical])!=(i<counts_[b]))return false;
        }
        for(std::size_t p=0;p<A_.size();++p){
            auto key=A_[p];if(!key)continue;
            ++occupied;
            if(!keys.insert(key).second)return false;
            if(!retrieval_.contains(key))return false;
            if(locate_present(key)!=p || !contains(key))return false;
            if(retrieval_.at(key)>=counts_[hash_bin(key)])return false;
        }
        if(occupied!=size())return false;
        for(std::size_t w=0;w<words();++w)if(read_ram(w)!=ram[w])return false;
        return true;
    }
    std::size_t physical_bytes()const{return A_.capacity()*sizeof(std::uint64_t);}
    std::size_t external_metadata_min_bytes()const{
        return masks_.size()*sizeof(std::uint64_t)+counts_.size()*sizeof(std::size_t)
           +retrieval_.size()*(sizeof(std::uint64_t)+sizeof(std::size_t));
    }
};
}
