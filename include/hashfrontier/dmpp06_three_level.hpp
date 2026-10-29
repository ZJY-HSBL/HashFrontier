#pragma once
// LATIN 2006 DMPP06 Section 3: finite three-level routing, not a succinct proof.
// Third tier uses std::unordered_map instead of the cited high-performance dictionary.
#include "hashfrontier/dmpp06_first_level.hpp"
#include "hashfrontier/dmpp06_second_level.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace hf {
class DMPP06ThreeLevel16 {
public:
 struct Found {std::size_t hashcode;std::uint16_t payload;unsigned level;};
 enum class Insert {first,second,third,already_present,overflow};
 struct Stats {std::uint64_t inserted_first=0,inserted_second=0,inserted_third=0,overflow_first=0,overflow_second=0,overflow_third=0,duplicates=0,updates=0,erases=0,lookups=0;};
private:
 struct Item {std::size_t code;std::uint16_t value;};
 DMPP06FirstLevel first_;
 DMPP06SecondLevel second_;
 std::unordered_map<std::uint16_t,Item> third_;
 std::vector<std::size_t> free_;
 std::size_t first_codes_,second_codes_,cap_,next_=0;
 mutable Stats stats_{};
public:
 DMPP06ThreeLevel16(unsigned b1,std::size_t codes,unsigned b2,std::size_t tiles,std::size_t levels,
 std::size_t capacity,std::uint16_t a1,std::uint16_t c1,std::uint16_t a2,std::uint16_t c2,std::uint64_t seed)
 :first_(b1,codes,a1,c1),second_(b2,tiles,levels,a2,c2,seed),
 first_codes_(first_.bucket_count()*first_.code_capacity()),second_codes_(second_.code_capacity()),cap_(capacity){
   if(cap_>65536)throw std::invalid_argument("third capacity exceeds 16-bit universe");
   third_.reserve(cap_);
 }
 std::size_t size()const{return first_.size()+second_.size()+third_.size();}
 std::size_t third_size()const{return third_.size();}
 std::size_t code_base_second()const{return first_codes_;}
 std::size_t code_base_third()const{return first_codes_+second_codes_;}
 std::size_t code_capacity()const{return first_codes_+second_codes_+cap_;}
 Stats stats()const{return stats_;}
 std::optional<Found> lookup(std::uint16_t k)const{
   ++stats_.lookups;
   if(auto x=first_.lookup(k))return Found{x->first,x->second,1};
   if(auto x=second_.lookup(k))return Found{first_codes_+x->stable_hashcode,x->payload,2};
   if(auto x=third_.find(k);x!=third_.end())return Found{code_base_third()+x->second.code,x->second.value,3};
   return std::nullopt;
 }
 Insert insert(std::uint16_t k,std::uint16_t v){
   if(lookup(k)){++stats_.duplicates;return Insert::already_present;}
   if(first_.insert(k,v)){++stats_.inserted_first;return Insert::first;}
   ++stats_.overflow_first;
   auto r=second_.insert(k,v);
   if(r==DMPP06SecondLevel::Insert::inserted){++stats_.inserted_second;return Insert::second;}
   if(r==DMPP06SecondLevel::Insert::already_present)throw std::logic_error("cross-tier duplicate");
   ++stats_.overflow_second;
   if(third_.size()==cap_){++stats_.overflow_third;return Insert::overflow;}
   std::size_t c;
   if(!free_.empty()){c=free_.back();free_.pop_back();}
   else if(next_<cap_)c=next_++;
   else throw std::logic_error("third allocator inconsistent");
   third_.emplace(k,Item{c,v});++stats_.inserted_third;return Insert::third;
 }
 bool update(std::uint16_t k,std::uint16_t v){
   bool ok=false;
   if(first_.lookup(k))ok=first_.update(k,v);
   else if(second_.lookup(k))ok=second_.update(k,v);
   else if(auto it=third_.find(k);it!=third_.end()){it->second.value=v;ok=true;}
   if(ok)++stats_.updates;return ok;
 }
 bool erase(std::uint16_t k){
   bool ok=false;
   if(first_.lookup(k))ok=first_.erase(k);
   else if(second_.lookup(k))ok=second_.erase(k);
   else if(auto it=third_.find(k);it!=third_.end()){
     free_.push_back(it->second.code);third_.erase(it);ok=true;
   }
   if(ok)++stats_.erases;return ok;
 }
 bool verify()const{
   if(!first_.verify()||!second_.verify())return false;
   if(third_.size()+free_.size()!=next_||next_>cap_)return false;
   std::unordered_set<std::size_t> codes;
   for(const auto& [k,item]:third_){
     if(item.code>=cap_||!codes.insert(item.code).second)return false;
     auto x=lookup(k);
     if(!x||x->level!=3||x->hashcode!=code_base_third()+item.code||x->payload!=item.value)return false;
   }
   for(auto code:free_)if(code>=next_||!codes.insert(code).second)return false;
   return codes.size()==next_;
 }
};
} // namespace hf
