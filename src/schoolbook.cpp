#include "karatsuba/schoolbook.hpp"

namespace km {

mpz_class schoolbook(const mpz_class& left, const mpz_class& right) {
  const mp_size_t leftSize = mpz_size(left.get_mpz_t());
  const mp_size_t rightSize = mpz_size(right.get_mpz_t());
  if (leftSize == 0 || rightSize == 0) {
    return 0;
  }

  // Ensure left has at least as many limbs as right to minimize outer loop
  // overhead
  if (leftSize < rightSize) {
    return km::schoolbook(right, left);
  }

  mpz_class product;
  const mp_size_t capacity = leftSize + rightSize;
  mp_limb_t* output = mpz_limbs_write(product.get_mpz_t(), capacity);
  const mp_limb_t* leftLimbs = mpz_limbs_read(left.get_mpz_t());
  const mp_limb_t* rightLimbs = mpz_limbs_read(right.get_mpz_t());

  output[leftSize] = mpn_mul_1(output, leftLimbs, leftSize, rightLimbs[0]);
  for (mp_size_t i = 1; i < rightSize; ++i) {
    output[i + leftSize] = mpn_addmul_1(output + i, leftLimbs, leftSize, rightLimbs[i]);
  }

  const bool negative = (sgn(left) < 0) != (sgn(right) < 0);
  mpz_limbs_finish(product.get_mpz_t(), negative ? -capacity : capacity);
  return product;
}

}  // namespace km
