#include <gmpxx.h>

#include "karatsuba/karatsuba.hpp"
#include "test_framework.hpp"

using namespace km;

TEST_CASE(Karatsuba, ZeroOperands) {
  ASSERT_EQ(multiply(0, 0), 0);
  ASSERT_EQ(multiply(0, 999999), 0);
  ASSERT_EQ(multiply(999999, 0), 0);
  ASSERT_EQ(multiply(0, -12345), 0);
  ASSERT_EQ(multiply(-12345, 0), 0);
}

TEST_CASE(Karatsuba, IdentityAndSmallOperands) {
  const mpz_class val("987654321098765432109876543210");
  ASSERT_EQ(multiply(val, 1), val);
  ASSERT_EQ(multiply(1, val), val);
  ASSERT_EQ(multiply(val, -1), -val);
  ASSERT_EQ(multiply(-1, val), -val);
  ASSERT_EQ(multiply(-val, -1), val);
}

TEST_CASE(Karatsuba, SignCombinations) {
  mpz_class a;
  mpz_class b;
  mpz_ui_pow_ui(a.get_mpz_t(), 2, 2500);  // 2500 bits
  a += 123456789;
  mpz_ui_pow_ui(b.get_mpz_t(), 3, 1500);  // ~2377 bits
  b += 987654321;

  const mpz_class expected = a * b;

  ASSERT_EQ(multiply(a, b), expected);
  ASSERT_EQ(multiply(-a, b), -expected);
  ASSERT_EQ(multiply(a, -b), -expected);
  ASSERT_EQ(multiply(-a, -b), expected);
}

TEST_CASE(Karatsuba, CutoffBoundaryTransitions) {
  gmp_randclass rng(gmp_randinit_mt);
  rng.seed(777UL);

  // Test specifically around DEFAULT_CUTOFF (32 limbs)
  const std::size_t cutoff = DEFAULT_CUTOFF;
  for (std::size_t limbs : {cutoff - 2, cutoff - 1, cutoff, cutoff + 1, cutoff + 2, cutoff * 2}) {
    const mp_bitcnt_t bits = static_cast<mp_bitcnt_t>(limbs) * GMP_NUMB_BITS;
    mpz_class x = rng.get_z_bits(bits);
    mpz_class y = rng.get_z_bits(bits);
    mpz_setbit(x.get_mpz_t(), bits - 1);
    mpz_setbit(y.get_mpz_t(), bits - 1);

    const mpz_class expected = x * y;
    ASSERT_EQ(multiply(x, y, cutoff), expected);
  }
}

TEST_CASE(Karatsuba, CustomCutoffValues) {
  gmp_randclass rng(gmp_randinit_mt);
  rng.seed(888UL);

  const mp_bitcnt_t bits = 64 * GMP_NUMB_BITS;  // 64 limbs
  mpz_class x = rng.get_z_bits(bits);
  mpz_class y = rng.get_z_bits(bits);
  const mpz_class expected = x * y;

  // Cutoff = 1 forces deep recursion to single-limb
  ASSERT_EQ(multiply(x, y, 1), expected);
  // Cutoff = 4
  ASSERT_EQ(multiply(x, y, 4), expected);
  // Cutoff = 16
  ASSERT_EQ(multiply(x, y, 16), expected);
  // Cutoff = 128 (larger than 64, falls back to schoolbook immediately)
  ASSERT_EQ(multiply(x, y, 128), expected);
}

TEST_CASE(Karatsuba, AsymmetricOperandSizes) {
  gmp_randclass rng(gmp_randinit_mt);
  rng.seed(999UL);

  // Very large left, small right
  const mpz_class large = rng.get_z_bits(100 * GMP_NUMB_BITS);
  const mpz_class small = rng.get_z_bits(2 * GMP_NUMB_BITS);
  const mpz_class medium = rng.get_z_bits(35 * GMP_NUMB_BITS);

  ASSERT_EQ(multiply(large, small), large * small);
  ASSERT_EQ(multiply(small, large), small * large);
  ASSERT_EQ(multiply(large, medium), large * medium);
  ASSERT_EQ(multiply(medium, large), medium * large);
}

TEST_CASE(Karatsuba, PowersOfTwoAndSpecialPatterns) {
  mpz_class p1;
  mpz_class p2;
  mpz_ui_pow_ui(p1.get_mpz_t(), 2, 3000);
  mpz_ui_pow_ui(p2.get_mpz_t(), 2, 5000);

  mpz_class expectedPow;
  mpz_ui_pow_ui(expectedPow.get_mpz_t(), 2, 8000);
  ASSERT_EQ(multiply(p1, p2), expectedPow);

  // All 1s (Mersenne-like)
  const mpz_class m1 = p1 - 1;
  const mpz_class m2 = p2 - 1;
  ASSERT_EQ(multiply(m1, m2), m1 * m2);
}

TEST_CASE(Karatsuba, RandomizedFuzzStress) {
  gmp_randclass rng(gmp_randinit_mt);
  rng.seed(54321UL);

  for (std::size_t i = 0; i < 75; ++i) {
    const mp_bitcnt_t bitsA = 100 + (i * 70);
    const mp_bitcnt_t bitsB = 80 + (i * 65);
    mpz_class a = rng.get_z_bits(bitsA);
    mpz_class b = rng.get_z_bits(bitsB);

    if (i % 2 == 1)
      a = -a;
    if (i % 3 == 2)
      b = -b;

    const mpz_class expected = a * b;
    const mpz_class actual = multiply(a, b);
    ASSERT_EQ(actual, expected);
  }
}

TEST_CASE(Karatsuba, ParallelMultiplication) {
  gmp_randclass rng(gmp_randinit_mt);
  rng.seed(12345UL);

  for (std::size_t limbs : {64, 128, 256, 512}) {
    const mp_bitcnt_t bits = static_cast<mp_bitcnt_t>(limbs) * GMP_NUMB_BITS;
    mpz_class x = rng.get_z_bits(bits);
    mpz_class y = rng.get_z_bits(bits);
    mpz_setbit(x.get_mpz_t(), bits - 1);
    mpz_setbit(y.get_mpz_t(), bits - 1);

    const mpz_class expected = x * y;
    const mpz_class actualParallel = multiply_parallel(x, y, DEFAULT_CUTOFF, 64);
    ASSERT_EQ(actualParallel, expected);
  }
}

TEST_CASE(Karatsuba, InPlaceAliasing) {
  gmp_randclass rng(gmp_randinit_mt);
  rng.seed(5432UL);

  for (std::size_t limbs : {16, 48, 128}) {
    const mp_bitcnt_t bits = static_cast<mp_bitcnt_t>(limbs) * GMP_NUMB_BITS;
    mpz_class a = rng.get_z_bits(bits);
    mpz_class b = rng.get_z_bits(bits);
    const mpz_class expected = a * b;

    mpz_class resA = a;
    multiply_impl(resA.get_mpz_t(), resA.get_mpz_t(), b.get_mpz_t(), DEFAULT_CUTOFF, 0, 0);
    ASSERT_EQ(resA, expected);

    mpz_class resB = b;
    multiply_impl(resB.get_mpz_t(), a.get_mpz_t(), resB.get_mpz_t(), DEFAULT_CUTOFF, 0, 0);
    ASSERT_EQ(resB, expected);

    // Square in place
    mpz_class resSq = a;
    multiply_impl(resSq.get_mpz_t(), resSq.get_mpz_t(), resSq.get_mpz_t(), DEFAULT_CUTOFF, 0, 0);
    ASSERT_EQ(resSq, a * a);
  }
}

TEST_CASE(Karatsuba, ExtremeAsymmetry) {
  gmp_randclass rng(gmp_randinit_mt);
  rng.seed(9999UL);

  for (std::size_t smallLimbs : {1, 33, 65}) {
    const mpz_class large = rng.get_z_bits(1000 * GMP_NUMB_BITS);
    const mpz_class small = rng.get_z_bits(smallLimbs * GMP_NUMB_BITS);
    const mpz_class expected = large * small;

    ASSERT_EQ(multiply(large, small, DEFAULT_CUTOFF), expected);
    ASSERT_EQ(multiply(small, large, DEFAULT_CUTOFF), expected);
  }
}
