#pragma once

#include "karatsuba/karatsuba.hpp"

inline constexpr std::size_t karatsubaSchoolbookCutoff = km::DEFAULT_CUTOFF;

inline mpz_class karatsuba(const mpz_class& left, const mpz_class& right,
                           std::size_t cutoff = km::DEFAULT_CUTOFF) {
  return km::multiply(left, right, cutoff);
}
