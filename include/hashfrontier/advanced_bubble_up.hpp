#pragma once
// Kuszmaul & Mitzenmacher, SODA 2025, Section 5.2: phase-based advanced bubble-up.
// Engineering instantiation of paper's insertion/query mechanism; SplitMix64 is
// NOT mathematically fully independent and finite d_core does NOT certify Thm 5.1.
// No deletion support: paper Appendix C's tombstone/rebuild algorithm is separate.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace hf {
class AdvancedBubbleUp {
public:
    struct Slot { std::uint64_t key=0; std::uint16_t choice=0; };
    struct Stats { std::uint64_t probes=0, noncore_probes=0, evictions=0, core_moves=0, failures=0; };
private:
    std::vector<Slot> table_;
    std::size_t count_=0, d_=0, core_=0, dmax_=0, phases_=1, max_core_chain_=0;
    double alpha_;
    std::uint64_t seed_=0, rng_=0;
    mutable Stats stat_{};
    static std::uint64_t mix(std::uint64_t x) {
        x+=0x9e3779b97f4a7c15ULL;
        x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;
        x=(x^(x>>27))*0x94d049bb133111ebULL;
        return x^(x>>31);
    }
    std::size_t pos(std::uint64_t key,std::size_t choice) const {
        return static_cast<std::size_t>(mix(key ^ mix(seed_ + 0xd6e8feb86659fd93ULL*choice)) % table_.size());
    }
    std::size_t core_choice() {
        rng_=mix(rng_);
        return dmax_ - static_cast<std::size_t>(rng_%core_);
    }
    void advance() {
        while(dmax_<d_ && double(count_)/table_.size() >= 1.0-std::exp(-double(dmax_)+alpha_)) {
            dmax_=std::min(d_,dmax_+core_); ++phases_;
        }
    }
public:
    // total_choices d must be >= d_core, otherwise the paper's core is undefined.
    // In experiments choose (d,d_core,alpha) explicitly; do not infer asymptotic
    // admissibility from small sample sizes.
    AdvancedBubbleUp(std::size_t capacity,std::size_t total_choices,
                     std::size_t d_core,double alpha,std::uint64_t seed,
                     std::size_t max_consecutive_core_moves=2048)
        :table_(capacity),d_(total_choices),core_(d_core),max_core_chain_(max_consecutive_core_moves),
         alpha_(alpha),seed_(seed),rng_(mix(seed^0xCAFED00D12345678ULL)) {
        if(capacity<2 || core_<2 || d_<core_ || d_>65535 || !(alpha_>0 && alpha_<1) ||
           max_core_chain_==0) throw std::invalid_argument("invalid advanced bubble-up parameters");
        const auto gamma = d_%core_;
        dmax_ = core_+gamma;
        if(dmax_>d_) dmax_=d_;
    }
    std::size_t size() const {return count_;}
    std::size_t capacity() const {return table_.size();}
    std::size_t max_choices() const {return d_;}
    std::size_t active_choices() const {return dmax_;}
    std::size_t phases() const {return phases_;}
    std::size_t memory_bytes() const {return table_.capacity()*sizeof(Slot);}
    Stats stats() const {return stat_;}
    void reset_stats() const {stat_={};}
    bool contains(std::uint64_t key) const {
        for(std::size_t j=dmax_;j>0;--j) {
            ++stat_.probes;
            const auto& slot=table_[pos(key,j)];
            if(slot.choice && slot.key==key) return true;
        }
        return false;
    }
    bool insert(std::uint64_t key) {
        if(contains(key) || count_==capacity()) return false;
        advance();
        const auto rng_start=rng_;
        const auto evict_start=stat_.evictions;
        const auto core_start=stat_.core_moves;
        const auto probes_start=stat_.probes;
        const auto first_start=stat_.noncore_probes;
        std::vector<std::pair<std::size_t,Slot>> undo;
        undo.reserve(std::min<std::size_t>(max_core_chain_+d_*4, table_.size()));
        auto rollback=[&] {
            for(auto it=undo.rbegin();it!=undo.rend();++it) table_[it->first]=it->second;
            rng_=rng_start; stat_.evictions=evict_start; stat_.core_moves=core_start;
            stat_.probes=probes_start; stat_.noncore_probes=first_start;
            ++stat_.failures;
            return false;
        };
        std::uint16_t choice=0;
        std::size_t core_streak=0;
        const std::size_t max_steps=std::max<std::size_t>(max_core_chain_*8,d_*32);
        for(std::size_t step=0;step<max_steps;++step) {
            std::size_t index=0,next_choice=0;
            const bool core_element=choice>dmax_-core_;
            if(core_element) {
                ++stat_.core_moves;
                if(++core_streak>max_core_chain_) return rollback();
                next_choice=core_choice(); index=pos(key,next_choice);
                ++stat_.probes;
            } else {
                core_streak=0;
                bool found=false;
                for(std::size_t j=std::max<std::size_t>(1,choice>core_?choice-core_:1);
                    j<=dmax_-core_;++j) {
                    index=pos(key,j); ++stat_.probes; ++stat_.noncore_probes;
                    if(!table_[index].choice) {next_choice=j;found=true;break;}
                }
                if(!found) {
                    next_choice=core_choice(); index=pos(key,next_choice);
                    ++stat_.probes; ++stat_.core_moves;
                }
            }
            Slot old=table_[index];
            undo.push_back({index,old});
            table_[index]={key,static_cast<std::uint16_t>(next_choice)};
            if(!old.choice) {++count_;return true;}
            ++stat_.evictions;key=old.key;choice=old.choice;
        }
        return rollback();
    }
    // Exhaustive validation of stored placement, without relying on hit queries.
    bool valid() const {
        std::size_t n=0;
        for(std::size_t i=0;i<table_.size();++i) {
            auto s=table_[i];if(!s.choice)continue;
            if(s.choice>dmax_||pos(s.key,s.choice)!=i) return false;
            ++n;
        }
        return n==count_;
    }
};
} // namespace hf
