#include "hashfrontier/partner_dictionary_section3.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>
#include <unordered_set>
#include <vector>
using Table=hf::PartnerDictionarySection3;
static void check(bool x,const char* s){if(!x)throw std::runtime_error(s);}
int main(){
 std::uint64_t checks=0, accepted=0, failed=0, reads=0, changed=0, uncouplings=0;
 for(const auto bins:{4UL,8UL,16UL,32UL})
 for(const auto B:{8UL,16UL,32UL})
 for(std::uint64_t seed=1;seed<=6;++seed){
  const auto n=bins*B;
  std::vector<std::vector<std::uint64_t>> pool(bins);
  // The finite-data test deliberately prepares balanced bins, rather than
  // claiming a random-key theorem requiring B >= log^4(n).
  Table probe(bins,B,seed, [&](){
      std::vector<std::uint64_t> tmp;
      const auto desired=3*B/4;
      // For probe construction compute the hash-bin formula directly.
      for(std::uint64_t k=1;tmp.size()<bins*desired;++k){
       std::size_t b=Table::hash_mix(k^Table::hash_mix(seed^0x9ae16a3b2f90404fULL))%bins;
       if(pool[b].size()<desired){pool[b].push_back(k);tmp.push_back(k);}
      }
      return tmp;
    }());
  std::unordered_set<std::uint64_t> oracle;
  for(auto &bin:pool)for(auto k:bin)oracle.insert(k);
  std::vector<std::optional<std::size_t>> ram(probe.words());
  check(probe.verify(ram),"init");++checks;
  std::mt19937_64 rng(seed*391+bins*19+B); // FIXED BELOW
  (void)n;
  for(int t=0;t<800;++t){
    const auto action=rng()%5;
    if(action<=1){
      auto word=rng()%probe.words();
      auto v=(rng()%5==0)?std::nullopt:std::optional<std::size_t>(rng()%probe.alphabet());
      bool result=probe.write_ram(word,v);
      if(result)ram[word]=v;else ++failed;
      check(probe.read_ram(word)==ram[word],"write roundtrip");++reads;++checks;
    } else if(action==2){
      auto it=oracle.begin();std::advance(it,rng()%oracle.size());
      auto key=*it;
      bool ok=probe.erase_key(key);
      if(ok){oracle.erase(key);++accepted;}else ++failed;
    } else {
      auto b=rng()%bins;
      std::uint64_t key=rng();
      for(int j=0;j<400 && (key==0||oracle.count(key)||probe.hash_bin(key)!=b);++j)key=rng();
      if(key && !oracle.count(key) && probe.hash_bin(key)==b){
        bool ok=probe.insert_key(key);
        if(ok){oracle.insert(key);++accepted;}else ++failed;
      }
    }
    if(t%20==0){
      check(probe.verify(ram),"full state invariant");++checks;
      for(int j=0;j<12;++j){auto k=rng();check(probe.contains(k)==bool(oracle.count(k)),"negative lookup");++checks;}
      for(int j=0;j<12;++j){auto it=oracle.begin();std::advance(it,rng()%oracle.size());check(probe.contains(*it),"positive lookup");++checks;}
    }
    ++changed;
  }
  check(probe.verify(ram),"final permutation");++checks;
  uncouplings+=probe.stats().decoupled_words;
  check(probe.size()==oracle.size(),"size/oracle");++checks;
 }
 check(uncouplings>0,"must exercise actual pair decoupling");++checks;
 bool invalid=false;
 try{Table x(3,16,1,{});}catch(const std::invalid_argument&){invalid=true;}
 check(invalid,"invalid bins rejected");
 std::cout<<"partner_dictionary_section3 OK checks="<<checks<<" changed="<<changed
          <<" inserts_deletes="<<accepted<<" explicitly_refused="<<failed
          <<" words_read="<<reads<<" decoupled_words="<<uncouplings<<"\n";
}
