#pragma once
// Independently implemented *basic* bubble-up insertion policy, SODA 2025,
// Kuszmaul & Mitzenmacher, Section 4, four move types.
// This is NOT the paper's advanced Section 5 algorithm and does NOT inherit
// its optimal-d high-probability theorem. Erase is a straightforward extension.
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <utility>
#include <algorithm>

namespace hf {
class BasicBubbleUp {
  struct Slot { std::uint64_t key=0; std::uint8_t choice=0; }; // 0 = empty
  std::vector<Slot> slots_;
  std::size_t count_=0, d_=0, core_limit_=0;
  std::uint64_t seed_=0;
  static std::uint64_t mix(std::uint64_t x) {
    x+=0x9e3779b97f4a7c15ULL;
    x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;
    x=(x^(x>>27))*0x94d049bb133111ebULL;
    return x^(x>>31);
  }
  // Independently keyed SplitMix-like hash evaluations are an engineering
  // approximation, NOT fully independent mathematical random functions.
  std::size_t pos(std::uint64_t key,std::size_t i) const {
    const auto salt=mix(seed_+i*0xd6e8feb86659fd93ULL);
    std::size_t p=static_cast<std::size_t>(mix(key^salt)%slots_.size());
    // Hash choices can collide within one key. Deterministic rejection keeps
    // different choices in different cells; it slightly changes the model.
    for(;;) {
      bool repeat=false;
      for(std::size_t j=1;j<i;j++) if(p==pos(key,j)) {repeat=true;break;}
      if(!repeat)return p;
      p=(p+1)%slots_.size();
    }
  }
  // Store a candidate in a free slot, or evict occupant and resume from its
  // previous choice. There is no permanent duplicate and size increments once.
  bool place(std::uint64_t key) {
    std::uint8_t choice=0;
    std::size_t core_streak=0;
    const std::size_t maximum_steps=slots_.size()*32;
    for(std::size_t step=0;step<maximum_steps;step++) {
      std::size_t destination=0,new_choice=0;
      bool free_found=false;
      if(choice==d_) {destination=pos(key,d_-1);new_choice=d_-1;++core_streak;}
      else if(choice==d_-1){destination=pos(key,d_);new_choice=d_;++core_streak;}
      else {
        core_streak=0;
        for(std::size_t j=static_cast<std::size_t>(choice)+1;j<=d_-2;j++) {
          auto p=pos(key,j);
          if(slots_[p].choice==0) {destination=p;new_choice=j;free_found=true;break;}
        }
        if(!free_found) {destination=pos(key,d_-1);new_choice=d_-1;}
      }
      if(core_streak>core_limit_) throw std::runtime_error("basic bubble-up core eviction limit");
      auto displaced=slots_[destination];
      slots_[destination]={key,static_cast<std::uint8_t>(new_choice)};
      if(displaced.choice==0)return true;
      key=displaced.key;choice=displaced.choice;
    }
    throw std::runtime_error("basic bubble-up maximum insertion steps");
  }
public:
  explicit BasicBubbleUp(std::size_t capacity,int load_pct,std::uint64_t seed)
      :slots_(capacity),seed_(seed) {
    if(capacity<128||load_pct<1||load_pct>97)throw std::invalid_argument("capacity/load");
    auto epsilon=1.0-static_cast<double>(load_pct)/100.0;
    d_=static_cast<std::size_t>(std::ceil(3*std::log(1.0/epsilon)))+1;
    if(d_>slots_.size()||d_<3||d_>255)throw std::invalid_argument("unsupported d");
    core_limit_=32*static_cast<std::size_t>(std::ceil(std::log2(capacity)))+32;
  }
  std::size_t size() const {return count_;}
  std::size_t capacity() const {return slots_.size();}
  std::size_t choices() const {return d_;}
  std::size_t memory_bytes() const {return slots_.capacity()*sizeof(Slot);}
  bool contains(std::uint64_t key) const {
    for(std::size_t j=d_;j>0;j--) {const auto&p=slots_[pos(key,j)];if(p.choice&&p.key==key)return true;}
    return false;
  }
  bool insert(std::uint64_t key) {
    if(contains(key))return false;
    if(count_==capacity())return false;
    if(!place(key))return false;
    ++count_;return true;
  }
  bool erase(std::uint64_t key) {
    for(std::size_t j=d_;j>0;j--) {
      auto &s=slots_[pos(key,j)];
      if(s.choice&&s.key==key) {s={};--count_;return true;}
    }
    return false;
  }
};
} // namespace hf
