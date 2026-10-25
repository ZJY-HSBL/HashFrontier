#include "hashfrontier/partner_ram_section3.hpp"
#include <cstdint>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>
#include <vector>
using hf::PartnerRAMSection3;
static void check(bool b,const char* msg){if(!b)throw std::runtime_error(msg);}
int main(){
    std::uint64_t checks=0, success=0, failed=0;
    for(const auto bins:{4UL,8UL,16UL,32UL,64UL})
    for(const auto B:{8UL,16UL,32UL})
    for(std::uint64_t seed=1;seed<=5;++seed){
        PartnerRAMSection3 h(bins,B,seed);
        std::vector<std::optional<std::size_t>> expected(h.num_words());
        check(h.verify(expected),"initial invariants");++checks;
        std::mt19937_64 rng(seed*2011+bins*11+B);
        for(int step=0;step<900;++step){
            const auto word=static_cast<std::size_t>(rng()%h.num_words());
            const bool erase=(rng()%4==0);
            auto value=erase?std::nullopt:std::optional<std::size_t>(rng()%h.word_alphabet());
            const bool ok=h.write(word,value);
            if(ok){expected[word]=value;++success;}else ++failed;
            check(h.read(word)==expected[word],"atomic write/read");++checks;
            const auto p=static_cast<std::size_t>(rng()%h.num_words());
            check(h.read(p)==expected[p],"random read");++checks;
            if(step%75==0){check(h.verify(expected),"permutation/partner invariant");++checks;}
        }
        check(h.verify(expected),"final invariants");++checks;
        std::size_t count=0;for(auto v:expected)if(v)++count;
        check(h.encoded_words()==count,"encoded count");++checks;
    }
    bool rejected=false;
    try{PartnerRAMSection3 bad(5,8,1);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"reject invalid bin count");
    std::cout<<"partner_ram_section3 OK checks="<<checks<<" accepted="<<success<<" unrepresentable="<<failed<<"\n";
}
