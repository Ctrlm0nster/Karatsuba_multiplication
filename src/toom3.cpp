#include "karatsuba/toom3.hpp"

#include <algorithm>
#include <cstring>
#include <future>

#include "karatsuba/limb_utils.hpp"
#include "karatsuba/thread_pool.hpp"

namespace km {

void toom3_impl(mpz_ptr rop, mpz_srcptr left, mpz_srcptr right, std::size_t kara_cutoff,
                std::size_t toom_cutoff, std::size_t parallel_cutoff, std::size_t depth) {
  if (mpz_sgn(left) == 0 || mpz_sgn(right) == 0) {
    mpz_set_ui(rop, 0);
    return;
  }

  // Handle in-place aliasing when output overlaps with operands
  if (rop == left || rop == right) {
    mpz_t tmp;
    mpz_init(tmp);
    toom3_impl(tmp, left, right, kara_cutoff, toom_cutoff, parallel_cutoff, depth);
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
    toom3_impl(rop, absLeft, absRight, kara_cutoff, toom_cutoff, parallel_cutoff, depth);
    if ((mpz_sgn(left) < 0) != (mpz_sgn(right) < 0)) {
      mpz_neg(rop, rop);
    }
    return;
  }

  const std::size_t leftSize = mpz_size(left);
  const std::size_t rightSize = mpz_size(right);

  // Ensure left has at least as many limbs as right
  if (leftSize < rightSize) {
    toom3_impl(rop, right, left, kara_cutoff, toom_cutoff, parallel_cutoff, depth);
    return;
  }

  // Base case: switch to Karatsuba when either operand fits within the toom_cutoff
  if (rightSize <= toom_cutoff) {
    multiply_impl(rop, left, right, kara_cutoff, parallel_cutoff, depth);
    return;
  }

  const std::size_t k = (leftSize + 2) / 3;

  // Split operands into 3 slices without heap allocation
  mpz_t x0;
  mpz_t x1;
  mpz_t x2;
  mpz_t y0;
  mpz_t y1;
  mpz_t y2;
  detail::get_slice_ro(left, 0, k, x0);
  detail::get_slice_ro(left, k, k, x1);
  detail::get_slice_ro(left, 2 * k, (leftSize > 2 * k) ? leftSize - 2 * k : 0, x2);

  detail::get_slice_ro(right, 0, k, y0);
  detail::get_slice_ro(right, k, (rightSize > k) ? std::min(k, rightSize - k) : 0, y1);
  detail::get_slice_ro(right, 2 * k, (rightSize > 2 * k) ? rightSize - 2 * k : 0, y2);

  const mp_bitcnt_t evalBits = static_cast<mp_bitcnt_t>(k + 2) * GMP_NUMB_BITS;
  const mp_bitcnt_t prodBits = static_cast<mp_bitcnt_t>(2 * k + 4) * GMP_NUMB_BITS;

  // Evaluation points: 1, -1, 2
  mpz_t p1;
  mpz_t q1;
  mpz_t pm1;
  mpz_t qm1;
  mpz_t p2;
  mpz_t q2;
  mpz_init2(p1, evalBits);
  mpz_init2(q1, evalBits);
  mpz_init2(pm1, evalBits);
  mpz_init2(qm1, evalBits);
  mpz_init2(p2, evalBits);
  mpz_init2(q2, evalBits);

  // P(1) = x2 + x1 + x0, Q(1) = y2 + y1 + y0
  mpz_add(p1, x2, x1);
  mpz_add(p1, p1, x0);
  mpz_add(q1, y2, y1);
  mpz_add(q1, q1, y0);

  // P(-1) = (x2 + x0) - x1, Q(-1) = (y2 + y0) - y1
  mpz_add(pm1, x2, x0);
  mpz_sub(pm1, pm1, x1);
  mpz_add(qm1, y2, y0);
  mpz_sub(qm1, qm1, y1);

  // P(2) = ((2*x2 + x1)*2) + x0, Q(2) = ((2*y2 + y1)*2) + y0
  mpz_mul_2exp(p2, x2, 1);
  mpz_add(p2, p2, x1);
  mpz_mul_2exp(p2, p2, 1);
  mpz_add(p2, p2, x0);

  mpz_mul_2exp(q2, y2, 1);
  mpz_add(q2, q2, y1);
  mpz_mul_2exp(q2, q2, 1);
  mpz_add(q2, q2, y0);

  // Pointwise product variables: v0, vinf, v1, vm1, v2
  mpz_t v0;
  mpz_t vinf;
  mpz_t v1;
  mpz_t vm1;
  mpz_t v2;
  mpz_init2(v0, prodBits);
  mpz_init2(vinf, prodBits);
  mpz_init2(v1, prodBits);
  mpz_init2(vm1, prodBits);
  mpz_init2(v2, prodBits);

  const bool canParallel = (parallel_cutoff > 0 && depth == 0 && rightSize >= parallel_cutoff);

  if (canParallel) {
    mpz_ptr pv0 = v0;
    mpz_ptr pvinf = vinf;
    mpz_ptr pv1 = v1;
    mpz_ptr pvm1 = vm1;

    auto f0 = ThreadPool::instance().enqueue(
        [pv0, x0, y0, kara_cutoff, toom_cutoff, parallel_cutoff, depth]() {
          toom3_impl(pv0, x0, y0, kara_cutoff, toom_cutoff, parallel_cutoff, depth + 1);
        });

    auto finf = ThreadPool::instance().enqueue(
        [pvinf, x2, y2, kara_cutoff, toom_cutoff, parallel_cutoff, depth]() {
          toom3_impl(pvinf, x2, y2, kara_cutoff, toom_cutoff, parallel_cutoff, depth + 1);
        });

    auto f1 = ThreadPool::instance().enqueue(
        [pv1, p1, q1, kara_cutoff, toom_cutoff, parallel_cutoff, depth]() {
          toom3_impl(pv1, p1, q1, kara_cutoff, toom_cutoff, parallel_cutoff, depth + 1);
        });

    auto fm1 = ThreadPool::instance().enqueue(
        [pvm1, pm1, qm1, kara_cutoff, toom_cutoff, parallel_cutoff, depth]() {
          toom3_impl(pvm1, pm1, qm1, kara_cutoff, toom_cutoff, parallel_cutoff, depth + 1);
        });

    try {
      toom3_impl(v2, p2, q2, kara_cutoff, toom_cutoff, parallel_cutoff, depth + 1);
      f0.get();
      finf.get();
      f1.get();
      fm1.get();
    } catch (...) {
      if (f0.valid()) {
        try {
          f0.wait();
        } catch (...) {
        }
      }
      if (finf.valid()) {
        try {
          finf.wait();
        } catch (...) {
        }
      }
      if (f1.valid()) {
        try {
          f1.wait();
        } catch (...) {
        }
      }
      if (fm1.valid()) {
        try {
          fm1.wait();
        } catch (...) {
        }
      }
      mpz_clear(p1);
      mpz_clear(q1);
      mpz_clear(pm1);
      mpz_clear(qm1);
      mpz_clear(p2);
      mpz_clear(q2);
      mpz_clear(v0);
      mpz_clear(vinf);
      mpz_clear(v1);
      mpz_clear(vm1);
      mpz_clear(v2);
      throw;
    }
  } else {
    toom3_impl(v0, x0, y0, kara_cutoff, toom_cutoff, parallel_cutoff, depth + 1);
    toom3_impl(vinf, x2, y2, kara_cutoff, toom_cutoff, parallel_cutoff, depth + 1);
    toom3_impl(v1, p1, q1, kara_cutoff, toom_cutoff, parallel_cutoff, depth + 1);
    toom3_impl(vm1, pm1, qm1, kara_cutoff, toom_cutoff, parallel_cutoff, depth + 1);
    toom3_impl(v2, p2, q2, kara_cutoff, toom_cutoff, parallel_cutoff, depth + 1);
  }

  // Interpolation:
  // w0 = v0
  // w4 = vinf
  // w2 = (v1 + vm1)/2 - vinf - v0
  // s1 = (v1 - vm1)/2 -> stored in w1
  // s2 = (v2 - v0 - 16*vinf - 4*w2)/2 -> stored in t
  // w3 = (s2 - s1)/3 = (t - w1)/3
  // w1 = s1 - w3 = w1 - w3

  mpz_t w2;
  mpz_t w3;
  mpz_t w1;
  mpz_t t;
  mpz_init2(w2, prodBits);
  mpz_init2(w3, prodBits);
  mpz_init2(w1, prodBits);
  mpz_init2(t, prodBits);

  // w2 = (v1 + vm1)/2 - vinf - v0
  mpz_add(t, v1, vm1);
  mpz_fdiv_q_2exp(t, t, 1);
  mpz_sub(w2, t, vinf);
  mpz_sub(w2, w2, v0);

  // w1 = s1 = (v1 - vm1)/2
  mpz_sub(t, v1, vm1);
  mpz_fdiv_q_2exp(w1, t, 1);

  // t = s2 = (v2 - v0 - 16*vinf - 4*w2)/2
  mpz_sub(t, v2, v0);
  mpz_mul_2exp(w3, vinf, 4);  // 16 * vinf
  mpz_sub(t, t, w3);
  mpz_mul_2exp(w3, w2, 2);  // 4 * w2
  mpz_sub(t, t, w3);
  mpz_fdiv_q_2exp(t, t, 1);

  // w3 = (s2 - s1)/3 = (t - w1)/3
  mpz_sub(t, t, w1);
  mpz_divexact_ui(w3, t, 3);

  // w1 = s1 - w3 = w1 - w3
  mpz_sub(w1, w1, w3);

  // Recombine into rop using direct limb additions
  const std::size_t totalLimbs = leftSize + rightSize + 4;
  mp_limb_t* out = mpz_limbs_write(rop, static_cast<mp_size_t>(totalLimbs));
  std::memset(out, 0, totalLimbs * sizeof(mp_limb_t));

  auto addToOut = [&](mpz_srcptr w, std::size_t offset) {
    const std::size_t sz = mpz_size(w);
    if (sz > 0 && offset < totalLimbs) {
      const std::size_t available = totalLimbs - offset;
      if (sz <= available) {
        if (mpz_sgn(w) >= 0) {
          mpn_add(out + offset, out + offset, available, mpz_limbs_read(w), sz);
        } else {
          mpn_sub(out + offset, out + offset, available, mpz_limbs_read(w), sz);
        }
      }
    }
  };

  addToOut(v0, 0);
  addToOut(w1, k);
  addToOut(w2, 2 * k);
  addToOut(w3, 3 * k);
  addToOut(vinf, 4 * k);

  mpz_limbs_finish(rop, static_cast<mp_size_t>(totalLimbs));

  mpz_clear(p1);
  mpz_clear(q1);
  mpz_clear(pm1);
  mpz_clear(qm1);
  mpz_clear(p2);
  mpz_clear(q2);
  mpz_clear(v0);
  mpz_clear(vinf);
  mpz_clear(v1);
  mpz_clear(vm1);
  mpz_clear(v2);
  mpz_clear(w2);
  mpz_clear(w3);
  mpz_clear(w1);
  mpz_clear(t);
}

mpz_class toom3(const mpz_class& left, const mpz_class& right, std::size_t kara_cutoff,
                std::size_t toom_cutoff, bool parallel, std::size_t parallel_cutoff) {
  mpz_class product;
  toom3_impl(product.get_mpz_t(), left.get_mpz_t(), right.get_mpz_t(), kara_cutoff, toom_cutoff,
             parallel ? parallel_cutoff : 0, 0);
  return product;
}

}  // namespace km
