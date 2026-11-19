#pragma once

#include <gmp.h>
#include <gmpxx.h>

#include <algorithm>
#include <cstddef>

namespace km::detail {

/**
 * @brief Initialize a read-only mpz_t view slicing limbs without allocation.
 *
 * @param src The source mpz_srcptr operand.
 * @param offset Starting limb index.
 * @param len Desired limb slice length.
 * @param ro Output mpz_t to initialize as read-only view.
 */
inline void get_slice_ro(mpz_srcptr src, std::size_t offset, std::size_t len, mpz_t ro) {
  static const mp_limb_t dummy_zero = 0;
  const std::size_t srcSize = mpz_size(src);
  if (offset >= srcSize || len == 0 || srcSize == 0) {
    mpz_roinit_n(ro, &dummy_zero, 0);
    return;
  }
  std::size_t actualLen = std::min(len, srcSize - offset);
  const mp_limb_t* ptr = mpz_limbs_read(src) + offset;
  while (actualLen > 0 && ptr[actualLen - 1] == 0) {
    --actualLen;
  }
  mpz_roinit_n(ro, (actualLen > 0) ? ptr : &dummy_zero, static_cast<mp_size_t>(actualLen));
}

/**
 * @brief Initialize a read-only mpz_t view of abs(src) without allocation.
 */
inline void get_abs_ro(mpz_srcptr src, mpz_t ro) {
  static const mp_limb_t dummy_zero = 0;
  const mp_size_t sz = static_cast<mp_size_t>(mpz_size(src));
  const mp_limb_t* ptr = (sz > 0) ? mpz_limbs_read(src) : &dummy_zero;
  mpz_roinit_n(ro, ptr, sz);
}

}  // namespace km::detail
