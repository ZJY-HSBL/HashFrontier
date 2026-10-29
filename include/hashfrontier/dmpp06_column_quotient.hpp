#pragma once
// DMPP06 LATIN 2006 Section 2.2, finite Transformations 2/3.
// NOT Siegel's high-independence family, Theorem 5 or a full DMPP06 table.
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
namespace hf {
class DMPP06ColumnQuotient16 {
  static constexpr unsigned CB=8, RB=8, C=1u<<CB, R=1u<<RB;
  unsigned bucket_bits_;
  std::array<std::uint8_t,R> row_shift_{};
  std::array<std::uint8_t,C> mul_{}, add_{}, inv_mul_{};
  static std::uint64_t mix(std::uint64_t& x) {
    x+=0x9e3779b97f4a7c15ULL;
    std::uint64_t z=x;
    z=(z^(z>>30))*0xbf58476d1ce4e5b9ULL;
    z=(z^(z>>27))*0x94d049bb133111ebULL;
    return z^(z>>31);
  }
  static std::uint8_t inverse_odd(std::uint8_t a) {
    for(unsigned i=1;i<256;i+=2)
      if((a*i & 255u)==1u)return static_cast<std::uint8_t>(i);
    throw std::logic_error("invalid odd permutation multiplier");
  }
public:
  explicit DMPP06ColumnQuotient16(unsigned bucket_bits,std::uint64_t seed):bucket_bits_(bucket_bits) {
    if(bucket_bits<8 || bucket_bits>15)
      throw std::invalid_argument("bucket bits must be 8..15");
    for(auto& v:row_shift_)v=static_cast<std::uint8_t>(mix(seed));
    for(unsigned c=0;c<C;++c){
      mul_[c]=static_cast<std::uint8_t>(mix(seed)|1u);
      add_[c]=static_cast<std::uint8_t>(mix(seed));
      inv_mul_[c]=inverse_odd(mul_[c]);
    }
  }
  unsigned bucket_count()const{return 1u<<bucket_bits_;}
  unsigned quotient_count()const{return 1u<<(16-bucket_bits_);}
  std::size_t metadata_bytes()const{
    return row_shift_.size()+mul_.size()+add_.size()+inv_mul_.size();
  }
  std::pair<std::uint16_t,std::uint16_t> quotient(std::uint16_t x)const{
    const unsigned row=x>>CB, col=x&255u;
    const unsigned shifted_col=(col+row_shift_[row])&255u;
    const unsigned transformed_row=(mul_[shifted_col]*row+add_[shifted_col])&255u;
    const unsigned within_col_bits=bucket_bits_-CB;
    const unsigned q_bits=RB-within_col_bits;
    const unsigned bucket=(shifted_col<<within_col_bits)|(transformed_row>>q_bits);
    const unsigned q=transformed_row & ((1u<<q_bits)-1);
    return {static_cast<std::uint16_t>(bucket),static_cast<std::uint16_t>(q)};
  }
  std::uint16_t restore(unsigned bucket,unsigned quotient)const{
    if(bucket>=bucket_count()||quotient>=quotient_count())
      throw std::out_of_range("bucket or quotient");
    const unsigned within_col_bits=bucket_bits_-CB;
    const unsigned q_bits=RB-within_col_bits;
    const unsigned shifted_col=bucket>>within_col_bits;
    const unsigned transformed_row=((bucket&((1u<<within_col_bits)-1))<<q_bits)|quotient;
    const unsigned row=(inv_mul_[shifted_col]*((transformed_row-add_[shifted_col])&255u))&255u;
    const unsigned col=(shifted_col-row_shift_[row])&255u;
    return static_cast<std::uint16_t>((row<<CB)|col);
  }
};
} // namespace hf
