#include "hashfrontier/partner_ram_section3.hpp"
#include <iostream>
#include <optional>
#include <stdexcept>
#include <vector>
using Opt=std::optional<std::size_t>;
static void require(bool b,const char*msg){if(!b)throw std::runtime_error(msg);}
static std::size_t tobin(const hf::PartnerRAMSection3&h,std::size_t w,std::size_t b){
 for(std::size_t v=0;v<h.word_alphabet();++v)if(h.destination_bin(w,v)==b)return v;
 throw std::runtime_error("bin encoding missing");
}
int main(){
 unsigned long checks=0,attempts=0,legacy_rejections=0,oversubscribed=0;
 for(auto bins:{8UL,16UL,32UL})for(auto B:{16UL,32UL,64UL})for(std::uint64_t seed=1;seed<=4;++seed){
  hf::PartnerRAMSection3 newer(bins,B,seed),older(bins,B,seed);
  std::vector<Opt> oracle(newer.num_words());
  const auto l=bins/2,r=l+1;
  for(std::size_t w=0;w<2*B;++w){
   auto val=tobin(newer,w,w<B?l:r);
   require(newer.write_batch_reuse({{w,val}}),"preparation failed");
   require(older.write_batch_disjoint({{w,val}}),"legacy preparation failed");
   oracle[w]=val;
  }
  require(newer.verify(oracle),"initial full permutation invalid");++checks;
  require(!older.write_batch_disjoint({{0,tobin(older,0,r)},{B,tobin(older,B,l)}}),"legacy incorrectly accepts full swap");
  ++legacy_rejections;
  for(std::size_t j=0;j<100;++j){
   auto a=j%B,b=B+j%B;
   auto dest_a=(j/B)%2==0?r:l,dest_b=(j/B)%2==0?l:r;
   const auto va=tobin(newer,a,dest_a),vb=tobin(newer,b,dest_b);
   require(newer.write_batch_reuse({{a,va},{b,vb}}),"prestate batch wrongly rejected");
   oracle[a]=va;oracle[b]=vb;++attempts;
   if(j%5==0){require(newer.verify(oracle),"batch corrupted logical words/permutation");++checks;}
  }
  require(newer.verify(oracle),"final permutation invalid");++checks;
  require(newer.batch_stats().reused_freed_partner>0,"freed partner not used");++checks;
  auto other=newer.destination_bin(0,*oracle[0])==l?r:l;
  require(!newer.write_batch_reuse({{0,tobin(newer,0,other)}}),"oversubscribed single update accepted");
  ++oversubscribed;
  require(newer.verify(oracle),"rejected batch changed physical state");++checks;
  bool threw=false;
  try{newer.write_batch_reuse({{0,Opt{}},{0,Opt{}}});}
  catch(const std::invalid_argument&){threw=true;}
  require(threw,"duplicate word accepted");++checks;
 }
 std::cout<<"checks="<<checks<<" atomic_mutations="<<attempts
          <<" old_rejections="<<legacy_rejections
          <<" capacity_rejections="<<oversubscribed<<" PASS\n";
}
