#pragma once

#include <gmp.h>
#include <gmpxx.h>

namespace km {

/**
 * @brief Low-level schoolbook multiplication writing directly into rop.
 */
void schoolbook_impl(mpz_ptr rop, mpz_srcptr left, mpz_srcptr right);

/**
 * @brief Multiplies two arbitrary-precision integers using classical
 * limb-level schoolbook (O(N * M)) multiplication.
 *
 * @param left First operand (mpz_class)
 * @param right Second operand (mpz_class)
 * @return mpz_class The product of left and right
 */
mpz_class schoolbook(const mpz_class& left, const mpz_class& right);

}  // namespace km
