#include "hashfrontier/siegel_tensor_product_finite.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <numeric>
#include <stdexcept>
#include <vector>
static void check(bool ok,const char* m){if(!ok)throw std::runtime_error(m);}
int main(){
 std::uint64_t checks=0,subsets=0,exact_cells=0;
 for(std::size_t u:{4UL,6UL,8UL})for(std::size_t d:{2UL,3UL})for(std::uint64_t s=1;s<=3;++s){
  auto [base,tries]=hf::SiegelFinitePeelable::find_certified(u,2*u,d,4,s);
  check(base.certify(4).k_peelable,"bad base graph");++checks;
  hf::SiegelTensorProductFinite p(std::move(base),2,s*1777+17);
  const auto c=p.certify(4);
  check(c.k_peelable,"product graph violates unique-neighbor property");++checks;subsets+=c.checked;
  for(std::size_t x=0;x<p.domain();++x){
    auto edges=p.neighbors(x);
    check(edges.size()==d*d,"wrong product degree");
    auto sorted=edges;std::sort(sorted.begin(),sorted.end());
    check(std::adjacent_find(sorted.begin(),sorted.end())==sorted.end(),"repeated product neighbor");
    std::uint64_t expected=0;
    for(auto v:edges){check(v<p.table_cells(),"neighbor outside right table");expected^=p.table_word(v);++checks;}
    check(expected==p.evaluate(x),"tensor evaluation mismatch");checks+=3;
  }
  check(p.adjacency_payload_bytes()==u*d*2,"wrong base graph payload");
  check(p.expanded_adjacency_payload_bytes()==u*u*d*d*2,"wrong expanded graph payload");checks+=2;
 }
 for(std::size_t u:{4UL,6UL})for(std::size_t d:{2UL,3UL})for(std::uint64_t s=1;s<=3;++s){
  auto [base,tries]=hf::SiegelFinitePeelable::find_certified(u,2*u,d,4,s);
  hf::SiegelTensorProductFinite p(std::move(base),3,s*17);
  const auto k=u==4?3u:2u;
  const auto c=p.certify(k);
  check(c.k_peelable,"cubic tensor certificate failed");++checks;subsets+=c.checked;
  for(std::size_t x=0;x<p.domain();++x){
    std::uint64_t expected=0;
    for(auto v:p.neighbors(x))expected^=p.table_word(v);
    check(expected==p.evaluate(x),"cubic evaluation mismatch");++checks;
  }
 }
 // Exact independence law by enumerating every one-bit right-table assignment.
 // A small tensor product with 3 right cells in each factor => 2^9 tables.
 auto [tiny,attempts]=hf::SiegelFinitePeelable::find_certified(2,3,2,2,0x5eed,1000);
 hf::SiegelTensorProductFinite p(std::move(tiny),2,3);
 check(p.certify(2).k_peelable,"tiny product not certified");++checks;
 for(std::size_t a=0;a<p.domain();++a)for(std::size_t b=a;b<p.domain();++b){
  if(a==b)continue;
  auto n1=p.neighbors(a),n2=p.neighbors(b);
  std::vector<unsigned> histogram(4,0);
  for(std::uint64_t mask=0;mask<(1ULL<<p.table_cells());++mask){
   unsigned x=0,y=0;
   for(auto i:n1)x^=static_cast<unsigned>((mask>>i)&1u);
   for(auto i:n2)y^=static_cast<unsigned>((mask>>i)&1u);
   ++histogram[(y<<1)|x];
  }
  for(auto v:histogram){check(v==(1u<<(p.table_cells()-2)),"2-wise exact joint law fails");++exact_cells;}
 }
 bool threw=false;try{p.neighbors(p.domain());}catch(const std::out_of_range&){threw=true;}
 check(threw,"out-of-range key accepted");++checks;
 std::cout<<"PASS checks="<<checks<<" certified_subsets="<<subsets<<" exact_uniformity_cells="<<exact_cells<<"\n";
}
