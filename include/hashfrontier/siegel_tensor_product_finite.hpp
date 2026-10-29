#pragma once
// Alan Siegel, SIAM J Comput 2004, Lemma 6: finite Cartesian/tensor power
// of an already-certified local graph. Finite unique-neighbor evaluation.
// DOES NOT reproduce the asymptotic efficient graph constructor or Siegel's
// high-independence bounds. Independent RIGHT TABLE values are mandatory.
#include "hashfrontier/siegel_peelable_finite.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace hf {
class SiegelTensorProductFinite {
public:
  struct Certificate { bool k_peelable; std::uint64_t checked; std::vector<std::size_t> witness; };
private:
  SiegelFinitePeelable base_;
  unsigned power_;
  std::size_t left_, right_, degree_;
  std::vector<std::uint64_t> random_table_;
  static std::uint64_t next(std::uint64_t& s){
    s += 0x9e3779b97f4a7c15ULL;
    auto z=s;
    z=(z^(z>>30))*0xbf58476d1ce4e5b9ULL;
    z=(z^(z>>27))*0x94d049bb133111ebULL;
    return z^(z>>31);
  }
  static std::size_t pow_small(std::size_t n,unsigned p){
    std::size_t v=1;
    for(unsigned i=0;i<p;++i){ if(n && v>65535/n)throw std::invalid_argument("product index exceeds uint16 range");v*=n; }
    return v;
  }
  template<typename F> void visit(std::size_t key,F&& fn)const {
    if(key>=left_)throw std::out_of_range("product key");
    std::array<std::size_t,3> digits{};
    auto rest=key;
    for(unsigned i=power_;i-- >0;){digits[i]=rest%base_.domain();rest/=base_.domain();}
    auto recurse=[&](auto&& self,unsigned level,std::size_t combined)->void{
      if(level==power_){fn(combined);return;}
      for(auto neighbor:base_.neighbors(digits.at(level)))
        self(self,level+1,combined*base_.table_cells()+neighbor);
    };
    recurse(recurse,0,0);
  }
public:
  SiegelTensorProductFinite(SiegelFinitePeelable base,unsigned power,std::uint64_t table_seed)
   :base_(std::move(base)),power_(power),left_(0),right_(0),degree_(0){
    if(power<2||power>3)throw std::invalid_argument("only finite powers 2 and 3");
    left_=pow_small(base_.domain(),power);
    right_=pow_small(base_.table_cells(),power);
    degree_=pow_small(base_.degree(),power);
    random_table_.resize(right_);
    for(auto& word:random_table_)word=next(table_seed);
  }
  std::size_t domain()const{return left_;}
  std::size_t table_cells()const{return right_;}
  std::size_t degree()const{return degree_;}
  std::size_t base_domain()const{return base_.domain();}
  std::size_t base_right()const{return base_.table_cells();}
  std::size_t base_degree()const{return base_.degree();}
  unsigned power()const{return power_;}
  std::size_t adjacency_payload_bytes()const{return base_.graph_minimum_bytes();}
  std::size_t expanded_adjacency_payload_bytes()const{return left_*degree_*sizeof(std::uint16_t);}
  std::size_t random_table_bytes()const{return right_*sizeof(std::uint64_t);}
  std::uint64_t table_word(std::size_t i)const{return random_table_.at(i);}
  std::vector<std::uint16_t> neighbors(std::size_t key)const{
    std::vector<std::uint16_t> result;result.reserve(degree_);
    visit(key,[&](std::size_t i){result.push_back(static_cast<std::uint16_t>(i));});
    return result;
  }
  std::uint64_t evaluate(std::size_t key)const{
    std::uint64_t result=0;
    visit(key,[&](std::size_t i){result^=random_table_[i];});
    return result;
  }
  Certificate certify(unsigned k)const {
    if(k==0||k>left_||k>4)throw std::invalid_argument("finite certificate only k 1..4");
    // Certification may materialize the expanded graph TEMPORARILY.
    // The measured hash evaluator never stores expanded adjacency.
    std::vector<std::vector<std::uint16_t>> edges(left_);
    for(std::size_t key=0;key<left_;++key)edges[key]=neighbors(key);
    std::vector<std::uint8_t> incidences(right_,0);
    std::vector<std::size_t> chosen;
    Certificate c{true,0,{}};
    auto enumerate=[&](auto&& self,std::size_t start,unsigned needed)->bool{
      if(needed==0){
        ++c.checked;
        for(auto key:chosen)for(auto col:edges[key])if(incidences[col]==1)return true;
        c.k_peelable=false;c.witness=chosen;return false;
      }
      for(std::size_t i=start;i+needed<=left_;++i){
        chosen.push_back(i);
        for(auto col:edges[i])++incidences[col];
        if(!self(self,i+1,needed-1))return false;
        for(auto col:edges[i])--incidences[col];
        chosen.pop_back();
      }
      return true;
    };
    for(unsigned sz=1;sz<=k;++sz)if(!enumerate(enumerate,0,sz))return c;
    return c;
  }
};
}
