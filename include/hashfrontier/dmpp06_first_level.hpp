#pragma once
// Demaine et al. LATIN 2006, Section 2.1 (finite affine field permutation)
// and Section 3.1 (first-level bucket filter, stable local hashcodes).
// This is NOT the complete DMPP06 dynamic retrieval construction: no
// Siegel hash family, no multi-level filters, no guaranteed succinct space.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>
namespace hf {
class DMPP06Affine16 {
    std::uint16_t a_,b_,inverse_a_;
    static std::uint16_t multiply(std::uint16_t a,std::uint16_t b) {
        std::uint16_t out=0;
        for(int j=0;j<16;++j){
            if(b&1)out^=a;
            bool high=(a&0x8000u)!=0;
            a=static_cast<std::uint16_t>(a<<1);
            if(high)a^=0x100bu; // x^16+x^12+x^3+x+1, irreducible over GF(2).
            b>>=1;
        }
        return out;
    }
    static std::uint16_t power(std::uint16_t a,std::uint32_t exp){
        std::uint16_t v=1;
        for(;exp;exp>>=1,a=multiply(a,a))if(exp&1)v=multiply(v,a);
        return v;
    }
public:
    DMPP06Affine16(std::uint16_t a,std::uint16_t b):a_(a),b_(b),inverse_a_(0){
        if(!a_)throw std::invalid_argument("zero affine coefficient");
        inverse_a_=power(a_,65534);
        if(multiply(a_,inverse_a_)!=1)throw std::logic_error("field polynomial/inverse invalid");
    }
    std::uint16_t forward(std::uint16_t key)const{return multiply(a_,key)^b_;}
    std::uint16_t inverse(std::uint16_t val)const{return multiply(inverse_a_,val^b_);}
    std::pair<std::uint16_t,std::uint16_t> quotient(std::uint16_t x,unsigned bucket_bits)const {
        if(bucket_bits<1||bucket_bits>15)throw std::invalid_argument("bucket_bits");
        auto y=forward(x);unsigned low=16-bucket_bits;
        return {static_cast<std::uint16_t>(y>>low),static_cast<std::uint16_t>(y&((1u<<low)-1u))};
    }
    std::uint16_t restore(std::uint16_t bucket,std::uint16_t q,unsigned bucket_bits)const{
        if(bucket_bits<1||bucket_bits>15||bucket>=(1u<<bucket_bits)||q>=(1u<<(16-bucket_bits)))
            throw std::invalid_argument("bad quotient address");
        return inverse(static_cast<std::uint16_t>((bucket<<(16-bucket_bits))|q));
    }
};
class DMPP06FirstLevel {
public:
    struct Stats{std::uint64_t inserted=0,removed=0,updated=0,overflow=0,lookups=0;};
private:
    struct Item{std::uint16_t local_code;std::uint16_t payload;};
    struct Bucket{
        std::unordered_map<std::uint16_t,Item> items;
        std::vector<std::uint16_t> free_codes;
    };
    DMPP06Affine16 hash_;
    unsigned bits_;
    std::size_t code_capacity_,size_=0;
    std::vector<Bucket> buckets_;
    mutable Stats stats_{};
public:
    DMPP06FirstLevel(unsigned bucket_bits,std::size_t per_bucket_codes,std::uint16_t a,std::uint16_t b)
       :hash_(a,b),bits_(bucket_bits),code_capacity_(per_bucket_codes),buckets_(std::size_t{1}<<bucket_bits){
        if(bucket_bits<1||bucket_bits>12||per_bucket_codes<1||per_bucket_codes>65535)
            throw std::invalid_argument("invalid first-level parameters");
        for(auto &bucket:buckets_){
            bucket.items.reserve(per_bucket_codes);
            bucket.free_codes.reserve(per_bucket_codes);
            for(std::size_t j=per_bucket_codes;j>0;--j)bucket.free_codes.push_back(static_cast<std::uint16_t>(j-1));
        }
    }
    std::size_t size()const{return size_;}
    std::size_t bucket_count()const{return buckets_.size();}
    std::size_t code_capacity()const{return code_capacity_;}
    Stats stats()const{return stats_;}
    std::optional<std::pair<std::uint32_t,std::uint16_t>> lookup(std::uint16_t key)const {
        ++stats_.lookups;
        const auto [b,q]=hash_.quotient(key,bits_);
        const auto &items=buckets_[b].items;
        auto it=items.find(q);
        if(it==items.end())return std::nullopt;
        return std::pair<std::uint32_t,std::uint16_t>{static_cast<std::uint32_t>(b*code_capacity_+it->second.local_code),it->second.payload};
    }
    // Return false on full bucket; upstream paper sends failed keys to second level.
    // Existing keys are idempotent, preserving stable code unless explicitly deleted.
    bool insert(std::uint16_t key,std::uint16_t value){
        const auto[b,q]=hash_.quotient(key,bits_);
        auto &bucket=buckets_[b];
        auto it=bucket.items.find(q);
        if(it!=bucket.items.end())return false;
        if(bucket.free_codes.empty()){++stats_.overflow;return false;}
        auto code=bucket.free_codes.back();
        bucket.items.emplace(q,Item{code,value});
        bucket.free_codes.pop_back();++size_;++stats_.inserted;
        return true;
    }
    bool update(std::uint16_t key,std::uint16_t value){
        const auto[b,q]=hash_.quotient(key,bits_);
        auto &items=buckets_[b].items;
        auto it=items.find(q);
        if(it==items.end())return false;
        it->second.payload=value;++stats_.updated;return true;
    }
    bool erase(std::uint16_t key){
        const auto[b,q]=hash_.quotient(key,bits_);
        auto &bucket=buckets_[b];
        auto it=bucket.items.find(q);
        if(it==bucket.items.end())return false;
        bucket.free_codes.push_back(it->second.local_code);
        bucket.items.erase(it);--size_;++stats_.removed;return true;
    }
    bool verify()const{
        std::size_t n=0;
        for(std::size_t k=0;k<buckets_.size();++k){
            const auto &bk=buckets_[k];n+=bk.items.size();
            if(bk.items.size()+bk.free_codes.size()!=code_capacity_)return false;
            std::vector<bool> seen(code_capacity_,false);
            for(auto c:bk.free_codes){if(c>=code_capacity_||seen[c])return false;seen[c]=true;}
            for(auto [q,item]:bk.items){
                if(item.local_code>=code_capacity_||seen[item.local_code])return false;
                seen[item.local_code]=true;
                const auto key=hash_.restore(static_cast<std::uint16_t>(k),q,bits_);
                if(hash_.quotient(key,bits_)!=std::pair<std::uint16_t,std::uint16_t>{static_cast<std::uint16_t>(k),q})return false;
            }
        }
        return n==size_;
    }
};
}
