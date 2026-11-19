#pragma once

#include <gmp.h>
#include <gmpxx.h>

#include <cstddef>

#include "karatsuba/karatsuba.hpp"

namespace km {

/// Default cutoff threshold in limbs below which Karatsuba multiplication is used.
inline constexpr std::size_t DEFAULT_TOOM_CUTOFF = 64;

/// Default limb count threshold above which multi-threaded parallelism is engaged in Toom-3.
inline constexpr std::size_t DEFAULT_TOOM_PARALLEL_CUTOFF = 256;

/**
 * @brief Low-level memory-optimized Toom-3 multiplication writing directly into rop.
 */
void toom3_impl(mpz_ptr rop, mpz_srcptr left, mpz_srcptr right, std::size_t kara_cutoff,
                std::size_t toom_cutoff, std::size_t parallel_cutoff, std::size_t depth);

/**
 * @brief Multiplies two arbitrary-precision integers using the Toom-3 (Toom-Cook 3-way)
 * algorithm, achieving O(N^(log3(5))) ~ O(N^1.465) complexity.
 *
 * Operands are split into 3 slices:
 *   X(t) = x2 * t^2 + x1 * t + x0
 *   Y(t) = y2 * t^2 + y1 * t + y0
 *
 * Evaluates product polynomial W(t) = X(t) * Y(t) at 5 points:
 *   0, 1, -1, 2, infinity
 *
 * Performs 5 sub-multiplications instead of 9, followed by exact sequence interpolation
 * and limb-level recombination.
 *
 * @param left First operand (mpz_class)
 * @param right Second operand (mpz_class)
 * @param kara_cutoff Cutoff below which schoolbook multiplication is used
 * @param toom_cutoff Cutoff below which Karatsuba multiplication is used
 * @param parallel Whether to enable parallel execution of recursive branches
 * @param parallel_cutoff Limb threshold to activate multi-threading
 * @return mpz_class The product of left and right
 */
mpz_class toom3(const mpz_class& left, const mpz_class& right,
                std::size_t kara_cutoff = DEFAULT_CUTOFF,
                std::size_t toom_cutoff = DEFAULT_TOOM_CUTOFF, bool parallel = false,
                std::size_t parallel_cutoff = DEFAULT_TOOM_PARALLEL_CUTOFF);

/**
 * @brief Parallel Toom-3 multiplication using thread pool for recursive sub-multiplications.
 *
 * @param left First operand (mpz_class)
 * @param right Second operand (mpz_class)
 * @param kara_cutoff Cutoff below which schoolbook is used
 * @param toom_cutoff Cutoff below which Karatsuba is used
 * @param parallel_cutoff Limb threshold to activate multi-threading
 * @return mpz_class The product of left and right
 */
inline mpz_class toom3_parallel(const mpz_class& left, const mpz_class& right,
                                std::size_t kara_cutoff = DEFAULT_CUTOFF,
                                std::size_t toom_cutoff = DEFAULT_TOOM_CUTOFF,
                                std::size_t parallel_cutoff = DEFAULT_TOOM_PARALLEL_CUTOFF) {
  return toom3(left, right, kara_cutoff, toom_cutoff, true, parallel_cutoff);
}

}  // namespace km
