#pragma once

#include <gmpxx.h>

#include <cstddef>

#include "karatsuba/schoolbook.hpp"

namespace km {

/// Default cutoff threshold in limbs. When the smaller operand has fewer or
/// equal limbs, recursion stops and schoolbook multiplication is used.
inline constexpr std::size_t DEFAULT_CUTOFF = 32;

/**
 * @brief Multiplies two arbitrary-precision integers using the Karatsuba
 * divide-and-conquer algorithm.
 *
 * For operands represented as:
 *   left  = x1 * 2^split + x0
 *   right = y1 * 2^split + y0
 *
 * Karatsuba computes:
 *   z2 = x1 * y1
 *   z0 = x0 * y0
 *   z1 = (x1 + x0) * (y1 + y0) - z2 - z0
 *   product = z2 * 2^(2*split) + z1 * 2^split + z0
 *
 * This reduces 4 sub-multiplications to 3, achieving O(N^1.585) time
 * complexity.
 *
 * @param left First operand (mpz_class)
 * @param right Second operand (mpz_class)
 * @param cutoff Cutoff threshold in GMP limbs below which schoolbook
 * multiplication is used
 * @return mpz_class The product of left and right
 */
mpz_class multiply(const mpz_class& left, const mpz_class& right,
                   std::size_t cutoff = DEFAULT_CUTOFF);

}  // namespace km
