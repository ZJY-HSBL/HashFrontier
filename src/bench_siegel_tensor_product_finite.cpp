#include "hashfrontier/siegel_tensor_product_finite.hpp"
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
using Clock=std::chrono::steady_clock;
int main(){
 std::cout<<"mode,base_left,base_right,degree,power,k,seed,left_count,right_count,certified_subsets,evaluations,ns_per_op,graph_payload_bytes,expanded_graph_payload_bytes,table_bytes,checksum,valid\n";
 for(std::size_t u:{4UL,6UL,8UL})for(std::size_t d:{2UL,3UL})
 for(unsigned power:{2u,3u})for(unsigned seed=1;seed<=5;++seed){
  if(power==3 && u==8)continue;
  auto [base,tries]=hf::SiegelFinitePeelable::find_certified(u,2*u,d,4,seed);
  hf::SiegelTensorProductFinite p(std::move(base),power,0x1abeLLU+seed*13+u*17+d*19+power);
  const unsigned k=power==2?4:(u==4?3:2);
  const auto c=p.certify(k);
  if(!c.k_peelable)throw std::logic_error("product graph not k-peelable");
  std::vector<std::vector<std::uint16_t>> expanded;
  expanded.reserve(p.domain());
  for(std::size_t x=0;x<p.domain();++x)expanded.push_back(p.neighbors(x));
  constexpr std::size_t count=150000;
  std::vector<std::uint16_t> queries(count);
  std::uint64_t rng=seed*991+u*127+d*31+power;
  for(auto &x:queries){rng=rng*2862933555777941757ULL+3037000493ULL;x=static_cast<std::uint16_t>(rng%p.domain());}
  for(const auto* mode:{"factorized","expanded"}){
   auto t0=Clock::now();
   std::uint64_t sum=0;
   for(auto key:queries){
    std::uint64_t h=0;
    if(mode[0]=='f')h=p.evaluate(key);
    else for(auto neighbor:expanded[key])h^=p.table_word(neighbor);
    sum+=(h^0x9e3779b97f4a7c15ULL);
   }
   auto t1=Clock::now();
   const double ns=std::chrono::duration<double,std::nano>(t1-t0).count()/count;
   std::cout<<mode<<','<<u<<','<<2*u<<','<<d<<','<<power<<','<<k<<','<<seed<<','<<p.domain()<<','<<p.table_cells()<<','<<c.checked<<','<<count<<','<<ns<<','
     <<(mode[0]=='f'?p.adjacency_payload_bytes():p.expanded_adjacency_payload_bytes())<<','
     <<p.expanded_adjacency_payload_bytes()<<','<<p.random_table_bytes()<<','<<sum<<",1\n";
  }
 }
}
