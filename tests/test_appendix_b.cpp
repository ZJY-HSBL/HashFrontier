#include "hashfrontier/advanced_bubble_up.hpp"
#include "hashfrontier/advanced_bubble_appendix_c.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_set>
#include <vector>

static void require(bool x,const char* msg){if(!x)throw std::runtime_error(msg);}
int main(){
    std::uint64_t checks=0, phase_changes=0, overlap_redirects=0, prime_evals=0;
    for(std::size_t capacity:{128ULL,256ULL,1024ULL,4096ULL})
    for(int load:{70,85,95})
    for(std::uint64_t seed=1;seed<=10;seed++){
        // d=9, core=3, alpha=.5: phase 1→2 threshold 1-exp(-2.5).
        hf::AdvancedBubbleUp table(capacity,9,3,.95,seed,4096,true);
        std::mt19937_64 gen(seed^capacity^std::uint64_t(load));
        std::unordered_set<std::uint64_t> oracle;
        std::vector<std::uint64_t> keys;
        const auto limit=capacity*std::size_t(load)/100;
        while(keys.size()<limit){auto k=gen();if(oracle.insert(k).second){keys.push_back(k);if(!table.insert(k)) {std::cerr<<"FAIL cap="<<capacity<<" load="<<load<<" seed="<<seed<<" inserted="<<keys.size()<<" dmax="<<table.active_choices()<<" phases="<<table.phases()<<"\n";return 2;}}}
        require(table.valid(),"B hash placement");
        require(table.size()==keys.size(),"B cardinality");
        if(load==95) require(table.phases()>=2,"phase transition at 95 percent");
        phase_changes+=(table.phases()>1);
        for(const auto key:keys){require(table.contains(key),"B false negative");++checks;}
        for(unsigned j=0;j<500;j++){auto k=gen();require(table.contains(k)==(oracle.count(k)>0),"B differential mismatch");++checks;}
        auto stats=table.stats();overlap_redirects+=stats.core_collision_redirects;prime_evals+=stats.choice_prime_evaluations;
    }
    // Entire deletion workflow with B choice' enabled via the wrapper configuration.
    for(std::uint64_t seed=1;seed<=6;seed++){
        hf::AdvancedBubbleAppendixC map(512,9,3,.95,seed,.65,.92,4096,true);
        std::mt19937_64 gen(seed*31);
        std::unordered_set<std::uint64_t> reference;
        for(int i=0;i<12000;i++){
            const std::uint64_t k=gen()%600;
            switch(gen()%4){
            case 0: { bool expected=reference.insert(k).second;bool actual=map.insert(k);if(expected!=actual){std::cerr<<"B+C fail seed="<<seed<<" i="<<i<<" key="<<k<<" size="<<reference.size()<<" actual="<<actual<<" expected="<<expected<<" tombstones="<<map.tombstones()<<" rebuilds="<<map.rebuilds()<<"\n";return 3;}break; }
            case 1: case 2: { bool expected=reference.erase(k)>0;bool actual=map.erase(k);require(expected==actual,"B+C erase mismatch");break; }
            default:require(map.contains(k)==(reference.count(k)>0),"B+C query mismatch");
            }
            require(map.valid()&&map.size()==reference.size(),"B+C invariant");
            ++checks;
        }
    }
    require(phase_changes>=40,"never exercised paper phase transitions");
    require(overlap_redirects>0,"Appendix B collision redirection not exercised");
    require(prime_evals>0,"Appendix B choice' retrieval not exercised");
    std::cout << "PASS checks="<<checks<<" phase_changes="<<phase_changes
              <<" overlap_redirects="<<overlap_redirects<<" choice_prime_evaluations="<<prime_evals<<"\n";
}
