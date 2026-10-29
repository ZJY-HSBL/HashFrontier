#include "hashfrontier/dmpp06_column_quotient.hpp"
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
static void check(bool v){
  if(!v)throw std::runtime_error("DMPP06 finite quotient bijection failed");
}
int main(){
 std::uint64_t checks=0;
 for(unsigned bits:{8u,9u,10u,11u,12u,13u,14u,15u})
  for(unsigned seed=1;seed<=6;++seed){
   hf::DMPP06ColumnQuotient16 h(bits,0xabcdeULL+seed);
   std::array<std::uint8_t,65536> seen{};
   for(unsigned x=0;x<65536;++x){
     auto [b,q]=h.quotient(static_cast<std::uint16_t>(x));
     check(b<h.bucket_count()&&q<h.quotient_count());++checks;
     const unsigned packed=b*h.quotient_count()+q;
     check(!seen[packed]);seen[packed]=1;++checks;
     check(h.restore(b,q)==x);++checks;
   }
   for(auto v:seen){check(v==1);++checks;}
   check(h.metadata_bytes()==1024);++checks;
   bool thrown=false;
   try{(void)h.restore(h.bucket_count(),0);}
   catch(const std::out_of_range&){thrown=true;}
   check(thrown);++checks;
 }
 for(unsigned b:{0u,7u,16u}){
   bool thrown=false;
   try{hf::DMPP06ColumnQuotient16 bad(b,1);}
   catch(const std::invalid_argument&){thrown=true;}
   check(thrown);++checks;
 }
 std::cout<<"PASS finite-16-bit Section 2.2 Transformations 2/3 checks="<<checks<<"\n";
}
