#include "hashfrontier/dmpp06_polynomial_quotient.hpp"
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
using H=hf::DMPP06PolynomialQuotient16;
static void check(bool ok,const char* s){if(!ok)throw std::runtime_error(s);}
int main() {
  uint64_t checks=0;
  for(unsigned a=1;a<256;++a) {
    check(H::gf_mul(static_cast<uint8_t>(a),H::gf_inverse(static_cast<uint8_t>(a)))==1,"GF inverse");++checks;
  }
  for(unsigned k:{2u,4u,8u,16u})for(unsigned seed=1;seed<=4;++seed)for(unsigned bits:{8u,10u,12u,15u}) {
    H h(bits,seed*1301+bits,k);
    std::vector<bool> seen(65536,false);
#ifdef HF_SAN_FAST
    const uint32_t limit=2048;
#else
    const uint32_t limit=65536;
#endif
    for(uint32_t key=0;key<limit;++key) {
      auto [b,q]=h.quotient(static_cast<uint16_t>(key));
      size_t index=size_t(b)*h.quotient_count()+q;
      check(index<65536&&!seen[index],"not a permutation"); seen[index]=true;
      check(h.restore(b,q)==key,"inverse failure");checks+=2;
    }
    if(limit==65536)for(bool v:seen){check(v,"not surjective");++checks;}
    check(h.metadata_bytes()==768+k,"metadata");++checks;
  }
  // Exhaustive pairwise independence of polynomial (a0+a1*x) at two distinct rows.
  std::vector<bool> outputs(65536,false);
  for(unsigned a0=0;a0<256;++a0)for(unsigned a1=0;a1<256;++a1){
    unsigned x=a0^H::gf_mul(static_cast<uint8_t>(a1),17);
    unsigned y=a0^H::gf_mul(static_cast<uint8_t>(a1),233);
    size_t i=x*256+y;check(!outputs[i],"pair-independence failure");outputs[i]=true;++checks;
  }
  for(bool v:outputs){check(v,"pair range missing");++checks;}
  for(unsigned bits:{0u,7u,16u}) {
    bool thrown=false;try{H h(bits,1);}catch(const std::invalid_argument&){thrown=true;}
    check(thrown,"invalid bits");++checks;
  }
  for(unsigned k:{0u,257u}) {
    bool thrown=false;try{H h(8,1,k);}catch(const std::invalid_argument&){thrown=true;}
    check(thrown,"invalid k");++checks;
  }
  H x(10,42,8);
  for(unsigned b:{1024u,4096u}){
    bool thrown=false;try{(void)x.restore(b,0);}catch(const std::out_of_range&){thrown=true;}
    check(thrown,"invalid bucket accepted");++checks;
  }
  std::cout<<"PASS checks="<<checks<<" GF256 exact finite bijection and exhaustive pairwise independence\n";
}
