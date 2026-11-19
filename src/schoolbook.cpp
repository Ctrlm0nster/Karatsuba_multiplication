#include "karatsuba/schoolbook.hpp"

namespace km {

void schoolbook_impl(mpz_ptr rop, mpz_srcptr left, mpz_srcptr right) {
  if (rop == left || rop == right) {
    mpz_t tmp;
    mpz_init(tmp);
    schoolbook_impl(tmp, left, right);
    mpz_set(rop, tmp);
    mpz_clear(tmp);
    return;
  }

  const mp_size_t leftSize = mpz_size(left);
  const mp_size_t rightSize = mpz_size(right);
  if (leftSize == 0 || rightSize == 0) {
    mpz_set_ui(rop, 0);
    return;
  }

  // Ensure left has at least as many limbs as right to minimize outer loop overhead
  if (leftSize < rightSize) {
    schoolbook_impl(rop, right, left);
    return;
  }

  const mp_size_t capacity = leftSize + rightSize;
  mp_limb_t* output = mpz_limbs_write(rop, capacity);
  const mp_limb_t* leftLimbs = mpz_limbs_read(left);
  const mp_limb_t* rightLimbs = mpz_limbs_read(right);

  output[leftSize] = mpn_mul_1(output, leftLimbs, leftSize, rightLimbs[0]);
  for (mp_size_t i = 1; i < rightSize; ++i) {
    output[i + leftSize] = mpn_addmul_1(output + i, leftLimbs, leftSize, rightLimbs[i]);
  }

  const bool negative = (mpz_sgn(left) < 0) != (mpz_sgn(right) < 0);
  mpz_limbs_finish(rop, negative ? -capacity : capacity);
}

mpz_class schoolbook(const mpz_class& left, const mpz_class& right) {
  mpz_class product;
  schoolbook_impl(product.get_mpz_t(), left.get_mpz_t(), right.get_mpz_t());
  return product;
}

}  // namespace km
