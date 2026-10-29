#pragma once
// DMPP06 §2.2 finite model: k-wise polynomial GF(256) shifts, field-affine columns.
// NOT Siegel's family, NOT Theorem 5 or full dynamic retrieval.
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>
namespace hf {
class DMPP06PolynomialQuotient16 {
  unsigned bits_;
  std::vector<uint8_t> coeff_;
  std::array<uint8_t,256> mult_{},add_{},inv_{};
  static uint64_t mix(uint64_t& s) {
    s+=0x9e3779b97f4a7c15ULL;
    auto x=s; x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;
    x=(x^(x>>27))*0x94d049bb133111ebULL; return x^(x>>31);
  }
public:
  static uint8_t gf_mul(uint8_t a,uint8_t b) {
    uint8_t z=0;
    for(unsigned i=0;i<8;++i) {
      if(b&1) z^=a;
      bool high=(a&0x80u)!=0;
      a=static_cast<uint8_t>(a<<1);
      if(high) a^=0x1bu;
      b=static_cast<uint8_t>(b>>1);
    } return z;
  }
  static uint8_t gf_pow(uint8_t x,unsigned p) {
    uint8_t z=1;
    for(;p;p>>=1,x=gf_mul(x,x))if(p&1)z=gf_mul(z,x);
    return z;
  }
  static uint8_t gf_inverse(uint8_t x) {
    if(!x)throw std::invalid_argument("inverse zero");
    return gf_pow(x,254);
  }
  DMPP06PolynomialQuotient16(unsigned bits,uint64_t seed,unsigned k=8):bits_(bits),coeff_(k) {
    if(bits<8||bits>15||k<1||k>256)throw std::invalid_argument("bucket bits or independence");
    for(auto& x:coeff_)x=static_cast<uint8_t>(mix(seed));
    for(unsigned c=0;c<256;++c) {
      uint8_t a=0;
      do{a=static_cast<uint8_t>(mix(seed));}while(!a);
      mult_[c]=a;add_[c]=static_cast<uint8_t>(mix(seed));
      inv_[c]=gf_inverse(a);
    }
  }
  unsigned bucket_count()const{return 1u<<bits_;}
  unsigned quotient_count()const{return 1u<<(16-bits_);}
  unsigned independence()const{return static_cast<unsigned>(coeff_.size());}
  size_t metadata_bytes()const{return coeff_.size()+3*256;}
  uint8_t row_shift(uint8_t row)const {
    uint8_t z=0;
    for(auto it=coeff_.rbegin();it!=coeff_.rend();++it)z=gf_mul(z,row)^*it;
    return z;
  }
  std::pair<uint16_t,uint16_t> quotient(uint16_t key)const {
    unsigned row=key>>8, col=key&255u;
    unsigned shifted=(col+row_shift(static_cast<uint8_t>(row)))&255u;
    unsigned r=gf_mul(mult_[shifted],static_cast<uint8_t>(row))^add_[shifted];
    unsigned incol=bits_-8, qb=8-incol;
    return {static_cast<uint16_t>((shifted<<incol)|(r>>qb)),
            static_cast<uint16_t>(r&((1u<<qb)-1u))};
  }
  uint16_t restore(unsigned bucket,unsigned q)const {
    if(bucket>=bucket_count()||q>=quotient_count())throw std::out_of_range("bucket/quotient");
    unsigned incol=bits_-8,qb=8-incol,c=bucket>>incol;
    unsigned encoded=((bucket&((1u<<incol)-1u))<<qb)|q;
    unsigned row=gf_mul(inv_[c],static_cast<uint8_t>(encoded^add_[c]));
    unsigned col=(c-row_shift(static_cast<uint8_t>(row)))&255u;
    return static_cast<uint16_t>((row<<8)|col);
  }
};
} // namespace hf
