#include "karatsuba/karatsuba.hpp"

#include <algorithm>

namespace km {

mpz_class multiply(const mpz_class& left, const mpz_class& right, std::size_t cutoff) {
  const std::size_t leftSize = mpz_size(left.get_mpz_t());
  const std::size_t rightSize = mpz_size(right.get_mpz_t());

  // Base case: switch to schoolbook when either operand fits within the cutoff
  if (std::min(leftSize, rightSize) <= cutoff) {
    return km::schoolbook(left, right);
  }

  // Handle negative operands
  if (sgn(left) < 0 || sgn(right) < 0) {
    mpz_class product = km::multiply(abs(left), abs(right), cutoff);
    if ((sgn(left) < 0) != (sgn(right) < 0)) {
      product = -product;
    }
    return product;
  }

  // Split operands in limbs, aligned to GMP limb bit width
  const mp_bitcnt_t split = (std::max(leftSize, rightSize) / 2) * GMP_NUMB_BITS;

  const mpz_class leftHigh = left >> split;
  const mpz_class rightHigh = right >> split;
  mpz_class leftLow;
  mpz_class rightLow;
  mpz_fdiv_r_2exp(leftLow.get_mpz_t(), left.get_mpz_t(), split);
  mpz_fdiv_r_2exp(rightLow.get_mpz_t(), right.get_mpz_t(), split);

  // Three recursive multiplications
  mpz_class highProduct = km::multiply(leftHigh, rightHigh, cutoff);
  const mpz_class lowProduct = km::multiply(leftLow, rightLow, cutoff);
  mpz_class middle = km::multiply(leftHigh + leftLow, rightHigh + rightLow, cutoff);

  middle -= highProduct;
  middle -= lowProduct;

  highProduct <<= (2 * split);
  middle <<= split;
  highProduct += middle;
  highProduct += lowProduct;

  return highProduct;
}

}  // namespace km
