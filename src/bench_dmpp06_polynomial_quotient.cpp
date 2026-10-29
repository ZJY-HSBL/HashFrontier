#include "hashfrontier/dmpp06_polynomial_quotient.hpp"
#include "hashfrontier/dmpp06_column_quotient.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>
using Clock=std::chrono::steady_clock;
template<class F>
void run(const char* method,unsigned k,unsigned bits,unsigned n,unsigned seed,const char* shape,F& h) {
  std::vector<uint16_t> keys(65536);
  std::iota(keys.begin(),keys.end(),0);
  if(shape[0]=='u') {
    std::mt19937 rng(seed*787+bits*131+n);
    std::shuffle(keys.begin(),keys.end(),rng);
  }
  std::vector<unsigned> count(h.bucket_count());
  uint64_t sum=0;
  auto t0=Clock::now();
  for(unsigned i=0;i<n;++i){auto[b,q]=h.quotient(keys[i]);++count[b];sum+=q;}
  auto t1=Clock::now();
  unsigned largest=0;size_t collisions=0;
  for(auto x:count){largest=std::max(largest,x);if(x>=2)collisions+=x;}
  std::cout<<method<<','<<k<<','<<bits<<','<<n<<','<<seed<<','<<shape<<','
           <<collisions<<','<<largest<<','<<h.metadata_bytes()<<','
           <<std::chrono::duration<double,std::nano>(t1-t0).count()/n<<','<<sum<<",1\n";
}
int main() {
  std::cout<<"method,k,bucket_bits,keys,seed,shape,collision_keys,max_bucket,metadata_bytes,ns_per_hash,checksum,valid\n";
  for(unsigned bits:{8u,10u,12u})for(unsigned n:{1024u,4096u})for(unsigned seed=1;seed<=5;++seed)
    for(const char* shape:{"contiguous","uniform"}) {
      hf::DMPP06ColumnQuotient16 old(bits,seed*837+bits);
      run("v022_explicit",0,bits,n,seed,shape,old);
      for(unsigned k:{2u,8u,16u}) {
        hf::DMPP06PolynomialQuotient16 h(bits,seed*837+bits,k);
        run("v023_poly",k,bits,n,seed,shape,h);
      }
    }
}
