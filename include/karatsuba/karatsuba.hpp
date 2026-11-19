#pragma once

#include <gmp.h>
#include <gmpxx.h>

#include <cstddef>

#include "karatsuba/schoolbook.hpp"

namespace km {

/// Default cutoff threshold in limbs below which schoolbook multiplication is used.
inline constexpr std::size_t DEFAULT_CUTOFF = 32;

/// Default limb count threshold above which multi-threaded parallelism is engaged.
inline constexpr std::size_t DEFAULT_PARALLEL_CUTOFF = 256;

/**
 * @brief Low-level memory-optimized Karatsuba multiplication writing directly into rop.
 */
void multiply_impl(mpz_ptr rop, mpz_srcptr left, mpz_srcptr right, std::size_t cutoff,
                   std::size_t parallel_cutoff, std::size_t depth);

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
 * @param cutoff Cutoff threshold in GMP limbs below which schoolbook is used
 * @param parallel Whether to enable parallel execution of recursive branches
 * @param parallel_cutoff Limb threshold to activate multi-threading
 * @return mpz_class The product of left and right
 */
mpz_class multiply(const mpz_class& left, const mpz_class& right,
                   std::size_t cutoff = DEFAULT_CUTOFF, bool parallel = false,
                   std::size_t parallel_cutoff = DEFAULT_PARALLEL_CUTOFF);

/**
 * @brief Parallel Karatsuba multiplication using thread pool for recursive sub-multiplications.
 *
 * @param left First operand (mpz_class)
 * @param right Second operand (mpz_class)
 * @param cutoff Cutoff threshold in GMP limbs below which schoolbook is used
 * @param parallel_cutoff Limb threshold to activate multi-threading
 * @return mpz_class The product of left and right
 */
inline mpz_class multiply_parallel(const mpz_class& left, const mpz_class& right,
                                   std::size_t cutoff = DEFAULT_CUTOFF,
                                   std::size_t parallel_cutoff = DEFAULT_PARALLEL_CUTOFF) {
  return multiply(left, right, cutoff, true, parallel_cutoff);
}

}  // namespace km
