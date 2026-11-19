#include "karatsuba/karatsuba.hpp"

#include <algorithm>
#include <cstring>
#include <future>

#include "karatsuba/limb_utils.hpp"
#include "karatsuba/thread_pool.hpp"

namespace km {

void multiply_impl(mpz_ptr rop, mpz_srcptr left, mpz_srcptr right, std::size_t cutoff,
                   std::size_t parallel_cutoff, std::size_t depth) {
  if (mpz_sgn(left) == 0 || mpz_sgn(right) == 0) {
    mpz_set_ui(rop, 0);
    return;
  }

  // Handle in-place aliasing when output overlaps with operands
  if (rop == left || rop == right) {
    mpz_t tmp;
    mpz_init(tmp);
    multiply_impl(tmp, left, right, cutoff, parallel_cutoff, depth);
    mpz_set(rop, tmp);
    mpz_clear(tmp);
    return;
  }

  // Handle negative operands via zero-allocation absolute-value views
  if (mpz_sgn(left) < 0 || mpz_sgn(right) < 0) {
    mpz_t absLeft;
    mpz_t absRight;
    detail::get_abs_ro(left, absLeft);
    detail::get_abs_ro(right, absRight);
    multiply_impl(rop, absLeft, absRight, cutoff, parallel_cutoff, depth);
    if ((mpz_sgn(left) < 0) != (mpz_sgn(right) < 0)) {
      mpz_neg(rop, rop);
    }
    return;
  }

  const std::size_t leftSize = mpz_size(left);
  const std::size_t rightSize = mpz_size(right);

  // Ensure left has at least as many limbs as right
  if (leftSize < rightSize) {
    multiply_impl(rop, right, left, cutoff, parallel_cutoff, depth);
    return;
  }

  // Base case: switch to schoolbook when either operand fits within the cutoff
  if (rightSize <= cutoff) {
    schoolbook_impl(rop, left, right);
    return;
  }

  // Split operands in limbs without heap allocation
  const std::size_t split = leftSize / 2;

  mpz_t lLow;
  mpz_t lHigh;
  mpz_t rLow;
  mpz_t rHigh;
  detail::get_slice_ro(left, 0, split, lLow);
  detail::get_slice_ro(left, split, leftSize - split, lHigh);
  detail::get_slice_ro(right, 0, split, rLow);
  detail::get_slice_ro(right, split, (rightSize > split) ? rightSize - split : 0, rHigh);

  const mp_bitcnt_t sumBits = static_cast<mp_bitcnt_t>(split + 2) * GMP_NUMB_BITS;
  const mp_bitcnt_t prodBits = static_cast<mp_bitcnt_t>(2 * split + 2) * GMP_NUMB_BITS;

  mpz_t z0;
  mpz_t z2;
  mpz_t z1;
  mpz_t lSum;
  mpz_t rSum;
  mpz_init2(z0, prodBits);
  mpz_init2(z2, prodBits);
  mpz_init2(z1, prodBits);
  mpz_init2(lSum, sumBits);
  mpz_init2(rSum, sumBits);

  mpz_add(lSum, lHigh, lLow);
  mpz_add(rSum, rHigh, rLow);

  const bool canParallel = (parallel_cutoff > 0 && depth == 0 && rightSize >= parallel_cutoff);

  if (canParallel) {
    mpz_ptr pz0 = z0;
    mpz_ptr pz2 = z2;
    auto f0 = ThreadPool::instance().enqueue([pz0, lLow, rLow, cutoff, parallel_cutoff, depth]() {
      multiply_impl(pz0, lLow, rLow, cutoff, parallel_cutoff, depth + 1);
    });

    auto f2 = ThreadPool::instance().enqueue([pz2, lHigh, rHigh, cutoff, parallel_cutoff, depth]() {
      multiply_impl(pz2, lHigh, rHigh, cutoff, parallel_cutoff, depth + 1);
    });

    try {
      multiply_impl(z1, lSum, rSum, cutoff, parallel_cutoff, depth + 1);
      f0.get();
      f2.get();
    } catch (...) {
      if (f0.valid()) {
        try {
          f0.wait();
        } catch (...) {
        }
      }
      if (f2.valid()) {
        try {
          f2.wait();
        } catch (...) {
        }
      }
      mpz_clear(z0);
      mpz_clear(z2);
      mpz_clear(z1);
      mpz_clear(lSum);
      mpz_clear(rSum);
      throw;
    }
  } else {
    multiply_impl(z0, lLow, rLow, cutoff, parallel_cutoff, depth + 1);
    multiply_impl(z2, lHigh, rHigh, cutoff, parallel_cutoff, depth + 1);
    multiply_impl(z1, lSum, rSum, cutoff, parallel_cutoff, depth + 1);
  }

  // middle = (x1 + x0) * (y1 + y0) - z2 - z0
  mpz_sub(z1, z1, z2);
  mpz_sub(z1, z1, z0);

  // Direct limb-level recombination: avoids bit shifts and intermediate copies
  const std::size_t totalLimbs = leftSize + rightSize + 2;
  mp_limb_t* out = mpz_limbs_write(rop, static_cast<mp_size_t>(totalLimbs));
  std::memset(out, 0, totalLimbs * sizeof(mp_limb_t));

  const std::size_t z0Size = mpz_size(z0);
  if (z0Size > 0) {
    std::memcpy(out, mpz_limbs_read(z0), z0Size * sizeof(mp_limb_t));
  }

  const std::size_t z2Size = mpz_size(z2);
  if (z2Size > 0) {
    std::memcpy(out + 2 * split, mpz_limbs_read(z2), z2Size * sizeof(mp_limb_t));
  }

  const std::size_t z1Size = mpz_size(z1);
  if (z1Size > 0) {
    if (mpz_sgn(z1) >= 0) {
      mpn_add(out + split, out + split, totalLimbs - split, mpz_limbs_read(z1), z1Size);
    } else {
      mpn_sub(out + split, out + split, totalLimbs - split, mpz_limbs_read(z1), z1Size);
    }
  }

  mpz_limbs_finish(rop, static_cast<mp_size_t>(totalLimbs));

  mpz_clear(z0);
  mpz_clear(z2);
  mpz_clear(z1);
  mpz_clear(lSum);
  mpz_clear(rSum);
}

mpz_class multiply(const mpz_class& left, const mpz_class& right, std::size_t cutoff, bool parallel,
                   std::size_t parallel_cutoff) {
  mpz_class product;
  multiply_impl(product.get_mpz_t(), left.get_mpz_t(), right.get_mpz_t(), cutoff,
                parallel ? parallel_cutoff : 0, 0);
  return product;
}

}  // namespace km
