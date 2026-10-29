#pragma once
// DMPP06 LATIN 2006 Section 2.2 Transformation 4: grouped uniform random
// permutations on first-order buckets. Finite 16-bit analogue ONLY.
// Siegel's high-independence row-shift hash, Theorem 5, and the paper's exact
// asymptotic parameter scaling are NOT implemented here.
#include "hashfrontier/dmpp06_column_quotient.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace hf {
class DMPP06GroupedQuotient16 {
    static constexpr unsigned groups_=16, cols_per_group_=16, tiles_=16;
    DMPP06ColumnQuotient16 base_;
    unsigned bits_;
    std::array<std::array<std::uint8_t,tiles_>,groups_> perm_{}, inverse_{};
    static std::uint64_t splitmix(std::uint64_t& s){
        s+=0x9e3779b97f4a7c15ULL;
        auto z=s;z=(z^(z>>30))*0xbf58476d1ce4e5b9ULL;
        z=(z^(z>>27))*0x94d049bb133111ebULL;
        return z^(z>>31);
    }
    std::uint16_t transform(std::uint16_t y,bool undo) const {
        const unsigned col=y>>8, row=y&255u;
        const unsigned g=col/cols_per_group_, tile=row>>4, offset=row&15u;
        const unsigned other=undo?inverse_[g][tile]:perm_[g][tile];
        return static_cast<std::uint16_t>((col<<8)|(other<<4)|offset);
    }
public:
    explicit DMPP06GroupedQuotient16(unsigned bucket_bits,std::uint64_t seed)
        :base_(bucket_bits,seed^0xd6e8feb86659fd93ULL),bits_(bucket_bits){
        if(bits_<9||bits_>15)throw std::invalid_argument("bucket bits must be 9..15");
        std::uint64_t rng=seed^0xa24baed4963ee407ULL;
        for(unsigned g=0;g<groups_;++g){
            for(unsigned i=0;i<tiles_;++i)perm_[g][i]=static_cast<std::uint8_t>(i);
            for(unsigned i=tiles_-1;i>0;--i){
                // Fisher-Yates; rejection prevents modulo bias.
                const std::uint64_t range=i+1;
                const std::uint64_t threshold=(0ULL-range)%range;
                std::uint64_t sample;
                do{sample=splitmix(rng);}while(sample<threshold);
                const unsigned j=static_cast<unsigned>(sample%range);
                auto tmp=perm_[g][i];perm_[g][i]=perm_[g][j];perm_[g][j]=tmp;
            }
            for(unsigned i=0;i<tiles_;++i)inverse_[g][perm_[g][i]]=static_cast<std::uint8_t>(i);
        }
    }
    unsigned bucket_count()const{return 1u<<bits_;}
    unsigned quotient_count()const{return 1u<<(16-bits_);}
    unsigned group_count()const{return groups_;}
    unsigned tiles_per_column()const{return tiles_;}
    std::size_t metadata_bytes()const{return base_.metadata_bytes()+sizeof(perm_)+sizeof(inverse_);}
    std::uint8_t permuted_tile(unsigned g,unsigned original_tile)const {
        if(g>=groups_||original_tile>=tiles_)throw std::out_of_range("tile");
        return perm_[g][original_tile];
    }
    // The pre-Transformation-4 layout; useful for exact oracle comparisons.
    std::uint16_t pre_group_position(std::uint16_t x) const {
        auto [bucket,q]=base_.quotient(x);
        return static_cast<std::uint16_t>((bucket<<(16-bits_))|q);
    }
    std::pair<std::uint16_t,std::uint16_t> quotient(std::uint16_t x)const{
        const auto y=transform(pre_group_position(x),false);
        return {static_cast<std::uint16_t>(y>>(16-bits_)),
                static_cast<std::uint16_t>(y&((1u<<(16-bits_))-1u))};
    }
    std::uint16_t restore(unsigned bucket,unsigned q)const{
        if(bucket>=bucket_count()||q>=quotient_count())throw std::out_of_range("bucket/quotient");
        const std::uint16_t y=static_cast<std::uint16_t>((bucket<<(16-bits_))|q);
        const auto original=transform(y,true);
        return base_.restore(original>>(16-bits_),original&((1u<<(16-bits_))-1u));
    }
};
} // namespace hf
