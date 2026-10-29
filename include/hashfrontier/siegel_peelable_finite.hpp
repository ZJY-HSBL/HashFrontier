#pragma once
// Siegel 2004, Lemma 2.6 (also explained in Thorup FOCS 2013, §I.B):
// if every nonempty X of at most k left vertices has a right neighbor with
// exactly one incident edge from X, XOR of independent random table words
// indexed by each left vertex's neighbors is k-wise independent.
// A *finite exact graph-certificate harness*, NOT the explicit/sublinear
// Siegel construction. Adjacency lists require O(universe * degree) space.
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <utility>
#include <algorithm>
#include <functional>
namespace hf {
class SiegelFinitePeelable {
 public:
  struct Certificate {bool k_peelable; std::uint64_t subsets_checked; std::vector<std::size_t> witness;};
 private:
  std::size_t u_,v_,degree_;
  std::vector<std::vector<std::uint16_t>> edges_;
  std::vector<std::uint64_t> table_;
  static std::uint64_t next(std::uint64_t& s) {
    s+=0x9e3779b97f4a7c15ULL;
    auto z=s;
    z=(z^(z>>30))*0xbf58476d1ce4e5b9ULL;
    z=(z^(z>>27))*0x94d049bb133111ebULL;
    return z^(z>>31);
  }
 public:
  SiegelFinitePeelable(std::size_t left,std::size_t right,std::size_t degree,
                       std::uint64_t graph_seed,std::uint64_t word_seed)
      :u_(left),v_(right),degree_(degree),edges_(left),table_(right){
    if(!left||!right||right>65535||!degree||degree>right||left>1000)
      throw std::invalid_argument("finite graph shape");
    for(auto& ns:edges_){
      while(ns.size()<degree_){
        auto idx=static_cast<std::uint16_t>(next(graph_seed)%right);
        if(std::find(ns.begin(),ns.end(),idx)==ns.end())ns.push_back(idx);
      }
    }
    for(auto& w:table_)w=next(word_seed);
  }
  std::size_t domain()const{return u_;}
  std::size_t table_cells()const{return v_;}
  std::size_t degree()const{return degree_;}
  std::size_t graph_minimum_bytes()const{return u_*degree_*sizeof(std::uint16_t);}
  std::size_t random_table_bytes()const{return v_*sizeof(std::uint64_t);}
  const std::vector<std::uint16_t>& neighbors(std::size_t key)const{
    if(key>=u_)throw std::out_of_range("key");
    return edges_[key];
  }
  std::uint64_t evaluate(std::size_t key)const{
    std::uint64_t out=0;
    for(auto pos:neighbors(key))out^=table_[pos];
    return out;
  }
  std::uint64_t evaluate_with_bitmask(std::size_t key,std::uint64_t mask)const{
    std::uint64_t out=0;
    for(auto pos:neighbors(key))out^=((mask>>pos)&1ULL);
    return out;
  }
  Certificate certify(std::size_t k)const{
    if(k<1||k>u_)throw std::invalid_argument("k outside domain");
    std::vector<std::uint16_t> incidences(v_,0);
    std::vector<std::size_t> chosen;
    Certificate c{true,0,{}};
    std::function<bool(std::size_t,std::size_t)> visit=[&](std::size_t at,std::size_t needed)->bool{
      if(needed==0){
        ++c.subsets_checked;
        bool unique=false;
        for(auto idx:chosen){
          for(auto target:edges_[idx]) if(incidences[target]==1){unique=true;break;}
          if(unique)break;
        }
        if(!unique){c.k_peelable=false;c.witness=chosen;return false;}
        return true;
      }
      for(std::size_t i=at;i+needed<=u_;++i){
        chosen.push_back(i);
        for(auto t:edges_[i])++incidences[t];
        if(!visit(i+1,needed-1))return false;
        for(auto t:edges_[i])--incidences[t];
        chosen.pop_back();
      }
      return true;
    };
    for(std::size_t sz=1;sz<=k;++sz)if(!visit(0,sz))return c;
    return c;
  }
  static std::pair<SiegelFinitePeelable,std::size_t> find_certified(
      std::size_t u,std::size_t v,std::size_t d,std::size_t k,
      std::uint64_t seed,std::size_t max_attempts=256){
    for(std::size_t i=1;i<=max_attempts;++i){
      auto graph_seed=next(seed);
      auto word_seed=next(seed);
      SiegelFinitePeelable f(u,v,d,graph_seed,word_seed);
      if(f.certify(k).k_peelable)return {std::move(f),i};
    }
    throw std::runtime_error("finite graph search exhausted, no certificate");
  }
};
}
