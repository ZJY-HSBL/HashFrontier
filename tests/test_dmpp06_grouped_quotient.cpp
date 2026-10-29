#include "hashfrontier/dmpp06_grouped_quotient.hpp"
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
static void demand(bool x,const char*msg){if(!x)throw std::runtime_error(msg);}
int main(){
    std::uint64_t checks=0,changed=0;
    for(unsigned bits:{9u,10u,11u,12u,13u})for(std::uint64_t seed=1;seed<=6;++seed){
        hf::DMPP06GroupedQuotient16 h(bits,seed);
        hf::DMPP06ColumnQuotient16 baseline(bits,seed^0xd6e8feb86659fd93ULL);
        demand(h.metadata_bytes()==baseline.metadata_bytes()+512,"metadata");++checks;
        for(unsigned g=0;g<16;++g){
            std::array<bool,16> used{};
            for(unsigned t=0;t<16;++t){
                auto p=h.permuted_tile(g,t);
                demand(p<16&&!used[p],"permutation not bijective");used[p]=true;++checks;
            }
        }
        std::vector<bool> seen(65536,false);
        for(std::uint32_t x=0;x<65536;++x){
            auto [b,q]=h.quotient(static_cast<std::uint16_t>(x));
            demand(b<h.bucket_count()&&q<h.quotient_count(),"range");++checks;
            demand(h.restore(b,q)==x,"roundtrip failed");++checks;
            const std::uint32_t y=(b<<(16-bits))|q;
            demand(!seen[y],"output collision");seen[y]=true;++checks;
            const auto pre=h.pre_group_position(static_cast<std::uint16_t>(x));
            auto [bb,qq]=baseline.quotient(static_cast<std::uint16_t>(x));
            demand(pre==((bb<<(16-bits))|qq),"base mismatch");++checks;
            demand((pre>>8)==(y>>8),"grouping moved column");++checks;
            demand((pre&15)==(y&15),"grouping moved intra-tile offset");++checks;
            if(pre!=y)++changed;
        }
        for(bool v:seen){demand(v,"missing permuted output");++checks;}
        for(unsigned g=0;g<16;++g)for(unsigned t=0;t<16;++t){
            // All columns in a group use exactly the same permutation.
            for(unsigned j=0;j<16;++j){
                const unsigned col=g*16+j,from=(col<<8)|(t<<4);
                // For each 16-bit input, inverse baseline gives pre-stage position.
                const auto key=baseline.restore(from>>(16-bits),from&((1u<<(16-bits))-1u));
                auto [bb,qq]=h.quotient(key);
                const auto out=(bb<<(16-bits))|qq;
                demand((out>>8)==col&&((out&255)>>4)==h.permuted_tile(g,t),"group sharing");++checks;
            }
        }
    }
    try{hf::DMPP06GroupedQuotient16 h(8,1);throw std::runtime_error("accepted bits8");}
    catch(const std::invalid_argument&){++checks;}
    try{hf::DMPP06GroupedQuotient16 h(10,1);h.restore(1024,0);throw std::runtime_error("accepted bad bucket");}
    catch(const std::out_of_range&){++checks;}
    demand(changed>100000,"transformation inert");++checks;
    std::cout<<"checks="<<checks<<" changed_positions="<<changed<<" status=PASS\n";
}
