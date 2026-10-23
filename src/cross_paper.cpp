// HashFrontier cross-paper CPU set benchmark, instrumenting unmodified author cores.
// Native ZombieHT must provide tests/hm_wrapper.h, include/, hm/pc/gqf/hashutil.
// Native TPHT must provide tpht.h and tpht.c, compiled without algorithm edits.
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <sstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>
#include <openssl/sha.h>
#include <sys/resource.h>
#include "hm_wrapper.h"
#include "hashfrontier/basic_bubble_up.hpp"
extern "C" {
#include "tpht.h"
}

using Clock=std::chrono::steady_clock;
using Key=std::uint64_t;
static Key mix(Key x) { x+=0x9e3779b97f4a7c15ULL; x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL; x=(x^(x>>27))*0x94d049bb133111ebULL; return x^(x>>31); }
static Key read64(std::istream &in){unsigned char b[8];in.read((char*)b,8);if(!in)throw std::runtime_error("truncated trace");Key x=0;for(int i=0;i<8;i++)x|=Key(b[i])<<(8*i);return x;}
static void write64(std::ostream &out,Key x){for(int i=0;i<8;i++){auto b=char(x&255);out.write(&b,1);x>>=8;}if(!out)throw std::runtime_error("write trace failed");}
static std::string digest(const std::string &data){unsigned char hash[SHA256_DIGEST_LENGTH];SHA256((const unsigned char*)data.data(),data.size(),hash);static const char*digits="0123456789abcdef";std::string v;for(auto b:hash){v+=digits[b>>4];v+=digits[b&15];}return v;}
struct Cycle{std::vector<Key> gone,added,hits,misses;};
struct Trace{Key capacity,load,seed;std::vector<Key> initial;std::vector<Cycle> cycles;std::string sha;};
static void generate(int argc,char**argv){
 if(argc!=8)throw std::runtime_error("generate capacity load_pct seed cycles queries output");
 const size_t cap=std::stoull(argv[2]),pct=std::stoull(argv[3]),seed=std::stoull(argv[4]),cycles=std::stoull(argv[5]),queries=std::stoull(argv[6]);
 if(cap<128||pct<1||pct>97||cycles<1||cycles>5000||queries<1||queries>100000)throw std::runtime_error("invalid trace arguments");
 size_t n=cap*pct/100;std::mt19937_64 rng(mix(seed));std::vector<Key> live;live.reserve(n);
 std::unordered_set<Key> used;used.reserve(n+cycles*(n/20+1));
 Key counter=seed*1000000000ULL+1;
 auto next_key=[&](){for(;;){Key k=mix(counter++);if(used.insert(k).second)return k;}};
 for(size_t i=0;i<n;i++)live.push_back(next_key());
 std::shuffle(live.begin(),live.end(),rng);
 std::ofstream out(argv[7],std::ios::binary);if(!out)throw std::runtime_error("trace file open failed");
 write64(out,0x48465f5443524143ULL);write64(out,1);write64(out,cap);write64(out,pct);write64(out,seed);write64(out,n);write64(out,cycles);write64(out,queries);
 for(Key k:live)write64(out,k);
 std::vector<size_t> idx(n);std::iota(idx.begin(),idx.end(),0);
 size_t edits=std::max<size_t>(1,n/20);
 for(size_t c=0;c<cycles;c++){
  std::shuffle(idx.begin(),idx.end(),rng);
  for(size_t j=0;j<edits;j++){auto slot=idx[j];Key old=live[slot],fresh=next_key();write64(out,old);write64(out,fresh);live[slot]=fresh;}
  std::uniform_int_distribution<size_t> pick(0,n-1);
  for(size_t j=0;j<queries;j++)write64(out,live[pick(rng)]);
  for(size_t j=0;j<queries;j++)write64(out,next_key());
 }
 std::cerr<<"generated cap="<<cap<<" load="<<pct<<" seed="<<seed<<" cycles="<<cycles<<" updates="<<edits<<" queries="<<queries<<"\n";
}
static Trace load_trace(const std::string&path){
 std::ifstream in(path,std::ios::binary);if(!in)throw std::runtime_error("missing trace");
 std::string bytes((std::istreambuf_iterator<char>(in)),{});Trace t{};t.sha=digest(bytes);
 std::stringbuf buf(bytes,std::ios::in|std::ios::binary);std::istream src(&buf);
 if(read64(src)!=0x48465f5443524143ULL||read64(src)!=1)throw std::runtime_error("invalid trace magic/version");
 t.capacity=read64(src);t.load=read64(src);t.seed=read64(src);Key n=read64(src),c=read64(src),q=read64(src);
 if(t.capacity>100000000||n==0||n>t.capacity||c>5000||q>100000)throw std::runtime_error("untrusted trace dimensions");
 t.initial.reserve(n);for(Key i=0;i<n;i++)t.initial.push_back(read64(src));
 const auto edits=std::max<Key>(1,n/20);
 for(Key i=0;i<c;i++){
  Cycle p; p.gone.reserve(edits);p.added.reserve(edits);p.hits.reserve(q);p.misses.reserve(q);
  for(Key j=0;j<edits;j++){p.gone.push_back(read64(src));p.added.push_back(read64(src));}
  for(Key j=0;j<q;j++)p.hits.push_back(read64(src));
  for(Key j=0;j<q;j++)p.misses.push_back(read64(src));
  t.cycles.push_back(std::move(p));
 }
 if(src.peek()!=EOF)throw std::runtime_error("unexpected trailing trace bytes");return t;
}
struct Table {virtual ~Table()=default;virtual bool insert(Key)=0;virtual bool erase(Key)=0;virtual bool has(Key)=0;virtual size_t memory()const=0;virtual size_t physical_capacity()const=0;};
struct Zombie:Table{
 size_t count=0;
 Zombie(const Trace&t){if(!g_init(t.capacity,64,0,float(t.load)/100.0f))throw std::runtime_error("zombie init failed");}
 ~Zombie(){g_destroy();}
 bool insert(Key k)override{int r=g_insert(k,0);if(r==QF_NO_SPACE)throw std::runtime_error("QF_NO_SPACE"); if(r>=0){++count;return true;}throw std::runtime_error("unexpected zombie insert status="+std::to_string(r));}
 bool erase(Key k)override{int r=g_remove(k);if(r==0){--count;return true;}throw std::runtime_error("unexpected zombie erase status="+std::to_string(r));}
 bool has(Key k)override{std::uint64_t out=0;int r=g_lookup(k,&out);if(r==0)return true;if(r==QF_DOESNT_EXIST)return false;throw std::runtime_error("unexpected zombie get status="+std::to_string(r));}
 size_t memory()const override{return g_memory_usage();}
 size_t physical_capacity()const override{return g_collect_metadata_stats();}
};
struct Flatten:Table{
 flatten_tpht64_t* t=nullptr;
 Flatten(const Trace&tr){tpht_options_t opt=tpht_default_options();opt.resize_mode=TPHT_FIXED;opt.value_size=1;opt.hash_seed=mix(tr.seed)|1;t=flatten_tpht64_create(tr.capacity,&opt);if(!t)throw std::runtime_error("flatten init failed");}
 ~Flatten(){flatten_tpht64_destroy(t);}
 bool insert(Key k)override{auto r=flatten_tpht64_put(t,k,1);if(r==TPHT_OK)return true;throw std::runtime_error("flatten put status="+std::to_string(r));}
 bool erase(Key k)override{return flatten_tpht64_remove(t,k)==TPHT_OK;}
 bool has(Key k)override{std::uint64_t v=0;auto r=flatten_tpht64_get(t,k,&v);if(r==TPHT_OK){if(v!=1)throw std::runtime_error("flatten corrupt value");return true;}if(r==TPHT_NOT_FOUND)return false;throw std::runtime_error("flatten get status="+std::to_string(r));}
 size_t memory()const override{return flatten_tpht64_memory_bytes(t);}
 size_t physical_capacity()const override{return flatten_tpht64_capacity(t);}
};
struct Chained:Table{
 chained_tpht64_t* t=nullptr;
 Chained(const Trace&tr){tpht_options_t opt=tpht_default_options();opt.resize_mode=TPHT_FIXED;opt.value_size=1;opt.hash_seed=mix(tr.seed)|1;t=chained_tpht64_create(tr.capacity,0,&opt);if(!t)throw std::runtime_error("chained init failed");}
 ~Chained(){chained_tpht64_destroy(t);}
 bool insert(Key k)override{auto r=chained_tpht64_put(t,k,1);if(r==TPHT_OK)return true;throw std::runtime_error("chained put status="+std::to_string(r));}
 bool erase(Key k)override{return chained_tpht64_remove(t,k)==TPHT_OK;}
 bool has(Key k)override{std::uint64_t v=0;auto r=chained_tpht64_get(t,k,&v);if(r==TPHT_OK){if(v!=1)throw std::runtime_error("chained corrupt value");return true;}if(r==TPHT_NOT_FOUND)return false;throw std::runtime_error("chained get status="+std::to_string(r));}
 size_t memory()const override{return chained_tpht64_memory_bytes(t);}
 size_t physical_capacity()const override{return chained_tpht64_capacity(t);}
};
struct BubbleBasic:Table{
 hf::BasicBubbleUp t;
 BubbleBasic(const Trace&tr):t(tr.capacity,int(tr.load),tr.seed){}
 bool insert(Key k)override{return t.insert(k);}
 bool erase(Key k)override{return t.erase(k);}
 bool has(Key k)override{return t.contains(k);}
 size_t memory()const override{return t.memory_bytes();}
 size_t physical_capacity()const override{return t.capacity();}
};
struct Standard:Table{
 std::unordered_set<Key> t;
 Standard(const Trace&tr){t.reserve(tr.capacity);}
 bool insert(Key k)override{return t.insert(k).second;}
 bool erase(Key k)override{return t.erase(k)==1;}
 bool has(Key k)override{return t.contains(k);}
 size_t memory()const override{return 0;/* not measured with allocator hooks */}
 size_t physical_capacity()const override{return t.bucket_count();}
};
static double ns(Clock::time_point a,Clock::time_point b){return std::chrono::duration<double,std::nano>(b-a).count();}
static double p99(std::vector<double>& v){if(v.empty())throw std::runtime_error("empty latency data");size_t i=(v.size()*99+99)/100-1;std::nth_element(v.begin(),v.begin()+i,v.end());return v[i];}
static void benchmark(const std::string&alg,const Trace&t){
 std::unique_ptr<Table> table;
 if(alg=="zombie")table=std::make_unique<Zombie>(t);
 else if(alg=="tpht_flatten")table=std::make_unique<Flatten>(t);
 else if(alg=="tpht_chained")table=std::make_unique<Chained>(t);
 else if(alg=="bubble_up_basic")table=std::make_unique<BubbleBasic>(t);
 else if(alg=="std_unordered_set")table=std::make_unique<Standard>(t);
 else throw std::runtime_error("unknown algorithm");
 std::unordered_set<Key> oracle;oracle.reserve(t.capacity);
 std::vector<double> latency;latency.reserve(t.initial.size()/20*t.cycles.size()*2);
 double build=0,insert=0,erase=0,hit=0,miss=0;size_t ni=0,nd=0,nh=0,nm=0;
 for(Key k:t.initial){auto a=Clock::now();bool b=table->insert(k);auto z=Clock::now();build+=ns(a,z);if(!b||!oracle.insert(k).second)throw std::runtime_error("build correctness failure");}
 for(const auto&cycle:t.cycles){
  for(Key k:cycle.gone){auto a=Clock::now();bool b=table->erase(k);auto z=Clock::now();double v=ns(a,z);erase+=v;latency.push_back(v);nd++;if(!b||oracle.erase(k)!=1)throw std::runtime_error("delete correctness failure");}
  for(Key k:cycle.added){auto a=Clock::now();bool b=table->insert(k);auto z=Clock::now();double v=ns(a,z);insert+=v;latency.push_back(v);ni++;if(!b||!oracle.insert(k).second)throw std::runtime_error("insert correctness failure");}
  for(Key k:cycle.hits){auto a=Clock::now();bool b=table->has(k);auto z=Clock::now();hit+=ns(a,z);nh++;if(!b||!oracle.contains(k))throw std::runtime_error("hit correctness failure");}
  for(Key k:cycle.misses){auto a=Clock::now();bool b=table->has(k);auto z=Clock::now();miss+=ns(a,z);nm++;if(b||oracle.contains(k))throw std::runtime_error("miss correctness failure");}
  if(oracle.size()!=t.initial.size())throw std::runtime_error("occupancy drift");
  for(Key k:oracle)if(!table->has(k))throw std::runtime_error("silent loss of live key");
 }
 auto usage=rusage{};getrusage(RUSAGE_SELF,&usage);
 std::cout<<alg<<",load_"<<t.load<<","<<t.seed<<","<<t.load<<","<<t.capacity<<","<<table->physical_capacity()<<","<<t.initial.size()<<","<<t.cycles.size()<<","<<ni<<","<<nh<<","<<nm<<","<<t.sha<<",complete,true,"<<std::fixed<<std::setprecision(5)
 <<build/t.initial.size()<<","<<hit/nh<<","<<miss/nm<<","<<insert/ni<<","<<erase/nd<<","<<p99(latency)<<","<<table->memory()<<","<<usage.ru_maxrss<<",\n";
}
int main(int argc,char**argv){try{
 if(argc>=2 && std::strcmp(argv[1],"generate")==0){generate(argc,argv);return 0;}
 if(argc!=4||std::strcmp(argv[1],"run")!=0)throw std::runtime_error("usage: generate capacity pct seed cycles queries file | run algorithm trace.bin");
 Trace t=load_trace(argv[3]);benchmark(argv[2],t);return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 2;}}
