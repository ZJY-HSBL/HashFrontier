#pragma once
// Demaine, Meyer auf der Heide, Pagh & Patrascu, LATIN 2006, Section 3.2.
// Finite implementation of the paper's multi-filter tile-placement mechanism:
// per-bucket permutation, decreasing tile counts, reversible in-tile offsets,
// stable tile labels, and explicit third-level overflow.
// This does NOT implement their Theorem 5 quotient hash family, word packing,
// high-probability guarantees or the third level. The affine permutations here
// are finite, invertible engineering instantiations, not the paper's exact family.
#include "hashfrontier/dmpp06_first_level.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

namespace hf {
class DMPP06SecondLevel {
public:
    struct Found { std::size_t stable_hashcode; std::uint16_t payload; std::size_t filter_level; };
    struct Stats {std::uint64_t inserted=0, erased=0, updated=0, rejected=0, lookups=0;};
    enum class Insert {inserted, already_present, overflow};
private:
    struct Tile { bool occupied=false; std::uint16_t offset=0, payload=0; };
    struct Level {
        std::uint32_t a, bias, ainverse, mask;
        std::size_t tile_count, tile_width, code_start;
        std::vector<Tile> tiles;
    };
    struct Bucket {std::vector<Level> levels;};
    DMPP06Affine16 outer_;
    unsigned bucket_bits_, quotient_bits_;
    std::size_t quotient_domain_, codes_per_bucket_=0, size_=0;
    std::vector<Bucket> buckets_;
    mutable Stats stats_{};

    static std::uint64_t mix(std::uint64_t x){
        x+=0x9e3779b97f4a7c15ULL;
        x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;
        x=(x^(x>>27))*0x94d049bb133111ebULL;
        return x^(x>>31);
    }
    static std::uint32_t inverse_odd(std::uint32_t odd){
        // Newton iteration modulo 2^32; masking recovers the smaller quotient domain.
        std::uint32_t x=odd;
        for(unsigned j=0;j<5;++j)x*=2u-odd*x;
        return x;
    }
    std::uint32_t permute(std::uint32_t q,const Level& l)const {
        return (l.a*q+l.bias)&l.mask;
    }
    std::uint32_t unpermute(std::uint32_t permuted,const Level& l)const{
        return ((permuted-l.bias)*l.ainverse)&l.mask;
    }
    std::pair<std::uint16_t,std::uint16_t> split(std::uint16_t k)const {
        return outer_.quotient(k,bucket_bits_);
    }
    std::optional<std::pair<std::size_t,std::size_t>> locate(std::uint16_t k)const{
        const auto [bucket,q]=split(k);
        const auto& levels=buckets_[bucket].levels;
        for(std::size_t i=0;i<levels.size();++i){
            const auto& lev=levels[i];
            const auto p=permute(q,lev);
            const auto tile=static_cast<std::size_t>(p)/lev.tile_width;
            const auto offset=static_cast<std::uint16_t>(p%lev.tile_width);
            const auto& t=lev.tiles[tile];
            if(t.occupied && t.offset==offset)return std::pair{i,tile};
        }
        return std::nullopt;
    }
public:
    DMPP06SecondLevel(unsigned bucket_bits,std::size_t first_tile_count,
                      std::size_t level_count,std::uint16_t outer_a,std::uint16_t outer_b,
                      std::uint64_t seed)
      :outer_(outer_a,outer_b),bucket_bits_(bucket_bits),
       quotient_bits_(16u-bucket_bits),quotient_domain_(std::size_t{1}<<quotient_bits_),
       buckets_(std::size_t{1}<<bucket_bits){
        if(bucket_bits<1||bucket_bits>12 || !level_count || level_count>16 ||
           !first_tile_count || (first_tile_count&(first_tile_count-1)) ||
           first_tile_count>quotient_domain_ || first_tile_count>>(level_count-1)==0)
            throw std::invalid_argument("bad second-level geometry");
        for(std::size_t i=0;i<level_count;++i)codes_per_bucket_+=first_tile_count>>i;
        const auto mask=static_cast<std::uint32_t>(quotient_domain_-1);
        for(std::size_t b=0;b<buckets_.size();++b){
            std::size_t code_start=0;
            for(std::size_t j=0;j<level_count;++j){
                const std::size_t tiles=first_tile_count>>j;
                auto random=mix(seed^mix(b+0xA67B31ULL)^mix(j+0x9281ULL));
                const auto a=static_cast<std::uint32_t>(random)|1u;
                const auto bias=static_cast<std::uint32_t>(mix(random+17))&mask;
                buckets_[b].levels.push_back(Level{a,bias,inverse_odd(a),mask,
                    tiles,quotient_domain_/tiles,code_start,std::vector<Tile>(tiles)});
                code_start+=tiles;
            }
        }
    }
    std::size_t size()const{return size_;}
    std::size_t bucket_count()const{return buckets_.size();}
    std::size_t codes_per_bucket()const{return codes_per_bucket_;}
    std::size_t code_capacity()const{return buckets_.size()*codes_per_bucket_;}
    std::size_t filter_levels()const{return buckets_.front().levels.size();}
    Stats stats()const{return stats_;}
    std::optional<Found> lookup(std::uint16_t k)const{
        ++stats_.lookups;
        auto loc=locate(k);
        if(!loc)return std::nullopt;
        const auto[b,q]=split(k);
        const auto& level=buckets_[b].levels[loc->first];
        const auto& tile=level.tiles[loc->second];
        return Found{static_cast<std::size_t>(b)*codes_per_bucket_+level.code_start+loc->second,
                     tile.payload,loc->first};
    }
    Insert insert(std::uint16_t key,std::uint16_t payload){
        if(locate(key))return Insert::already_present;
        const auto[b,q]=split(key);
        for(auto& lev:buckets_[b].levels){
            const auto p=permute(q,lev);
            auto& tile=lev.tiles[p/lev.tile_width];
            if(tile.occupied)continue;
            tile={true,static_cast<std::uint16_t>(p%lev.tile_width),payload};
            ++size_;++stats_.inserted;
            return Insert::inserted;
        }
        ++stats_.rejected;
        return Insert::overflow;
    }
    bool update(std::uint16_t k,std::uint16_t payload){
        auto loc=locate(k);if(!loc)return false;
        auto [b,q]=split(k);(void)q;
        buckets_[b].levels[loc->first].tiles[loc->second].payload=payload;
        ++stats_.updated;return true;
    }
    bool erase(std::uint16_t k){
        auto loc=locate(k);if(!loc)return false;
        auto[b,q]=split(k);(void)q;
        buckets_[b].levels[loc->first].tiles[loc->second]={};
        --size_;++stats_.erased;return true;
    }
    bool verify()const{
        std::unordered_set<std::uint16_t> keys;
        std::unordered_set<std::size_t> codes;
        for(std::size_t b=0;b<buckets_.size();++b){
            const auto& lvls=buckets_[b].levels;
            for(std::size_t i=0;i<lvls.size();++i){
                const auto& l=lvls[i];
                if(l.tile_count*l.tile_width!=quotient_domain_)return false;
                for(std::size_t t=0;t<l.tiles.size();++t){
                    const auto& tile=l.tiles[t];if(!tile.occupied)continue;
                    if(tile.offset>=l.tile_width)return false;
                    const auto q=unpermute(static_cast<std::uint32_t>(t*l.tile_width+tile.offset),l);
                    const auto key=outer_.restore(static_cast<std::uint16_t>(b),static_cast<std::uint16_t>(q),bucket_bits_);
                    if(!keys.insert(key).second)return false;
                    const auto code=b*codes_per_bucket_+l.code_start+t;
                    if(!codes.insert(code).second)return false;
                    auto found=lookup(key);
                    if(!found || found->stable_hashcode!=code||found->payload!=tile.payload||found->filter_level!=i)
                        return false;
                }
            }
        }
        return keys.size()==size_;
    }
};
} // namespace hf
