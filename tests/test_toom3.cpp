#include <gmpxx.h>

#include "karatsuba/toom3.hpp"
#include "test_framework.hpp"

using namespace km;

TEST_CASE(Toom3, ZeroOperands) {
  ASSERT_EQ(toom3(0, 0), 0);
  ASSERT_EQ(toom3(0, 999999), 0);
  ASSERT_EQ(toom3(999999, 0), 0);
  ASSERT_EQ(toom3(0, -12345), 0);
  ASSERT_EQ(toom3(-12345, 0), 0);
}

TEST_CASE(Toom3, IdentityAndSmallOperands) {
  const mpz_class val("987654321098765432109876543210");
  ASSERT_EQ(toom3(val, 1), val);
  ASSERT_EQ(toom3(1, val), val);
  ASSERT_EQ(toom3(val, -1), -val);
  ASSERT_EQ(toom3(-1, val), -val);
  ASSERT_EQ(toom3(-val, -1), val);
}

TEST_CASE(Toom3, SignCombinations) {
  mpz_class a;
  mpz_class b;
  mpz_ui_pow_ui(a.get_mpz_t(), 2, 2500);  // 2500 bits
  a += 123456789;
  mpz_ui_pow_ui(b.get_mpz_t(), 3, 1500);  // ~2377 bits
  b += 987654321;

  const mpz_class expected = a * b;

  ASSERT_EQ(toom3(a, b), expected);
  ASSERT_EQ(toom3(-a, b), -expected);
  ASSERT_EQ(toom3(a, -b), -expected);
  ASSERT_EQ(toom3(-a, -b), expected);
}

TEST_CASE(Toom3, CutoffBoundaryTransitions) {
  gmp_randclass rng(gmp_randinit_mt);
  rng.seed(777UL);

  const std::size_t toomCutoff = DEFAULT_TOOM_CUTOFF;
  for (std::size_t limbs : {toomCutoff - 2, toomCutoff - 1, toomCutoff, toomCutoff + 1,
                            toomCutoff + 2, toomCutoff * 2}) {
    const mp_bitcnt_t bits = static_cast<mp_bitcnt_t>(limbs) * GMP_NUMB_BITS;
    mpz_class x = rng.get_z_bits(bits);
    mpz_class y = rng.get_z_bits(bits);
    mpz_setbit(x.get_mpz_t(), bits - 1);
    mpz_setbit(y.get_mpz_t(), bits - 1);

    const mpz_class expected = x * y;
    ASSERT_EQ(toom3(x, y, DEFAULT_CUTOFF, toomCutoff), expected);
  }
}

TEST_CASE(Toom3, CustomCutoffValues) {
  gmp_randclass rng(gmp_randinit_mt);
  rng.seed(888UL);

  const mp_bitcnt_t bits = 96 * GMP_NUMB_BITS;  // 96 limbs
  mpz_class x = rng.get_z_bits(bits);
  mpz_class y = rng.get_z_bits(bits);
  const mpz_class expected = x * y;

  // Cutoff = 1 forces deep recursion to single-limb
  ASSERT_EQ(toom3(x, y, 1, 1), expected);
  // Cutoff = 8
  ASSERT_EQ(toom3(x, y, 4, 8), expected);
  // Cutoff = 32
  ASSERT_EQ(toom3(x, y, 16, 32), expected);
  // Cutoff = 256 (larger than 96, falls back to Karatsuba immediately)
  ASSERT_EQ(toom3(x, y, 32, 256), expected);
}

TEST_CASE(Toom3, AsymmetricOperandSizes) {
  gmp_randclass rng(gmp_randinit_mt);
  rng.seed(999UL);

  const mpz_class large = rng.get_z_bits(120 * GMP_NUMB_BITS);
  const mpz_class small = rng.get_z_bits(2 * GMP_NUMB_BITS);
  const mpz_class medium = rng.get_z_bits(50 * GMP_NUMB_BITS);

  ASSERT_EQ(toom3(large, small), large * small);
  ASSERT_EQ(toom3(small, large), small * large);
  ASSERT_EQ(toom3(large, medium), large * medium);
  ASSERT_EQ(toom3(medium, large), medium * large);
}

TEST_CASE(Toom3, PowersOfTwoAndSpecialPatterns) {
  mpz_class p1;
  mpz_class p2;
  mpz_ui_pow_ui(p1.get_mpz_t(), 2, 3000);
  mpz_ui_pow_ui(p2.get_mpz_t(), 2, 5000);

  mpz_class expectedPow;
  mpz_ui_pow_ui(expectedPow.get_mpz_t(), 2, 8000);
  ASSERT_EQ(toom3(p1, p2), expectedPow);

  // All 1s (Mersenne-like)
  const mpz_class m1 = p1 - 1;
  const mpz_class m2 = p2 - 1;
  ASSERT_EQ(toom3(m1, m2), m1 * m2);
}

TEST_CASE(Toom3, RandomizedFuzzStress) {
  gmp_randclass rng(gmp_randinit_mt);
  rng.seed(54321UL);

  for (std::size_t i = 0; i < 75; ++i) {
    const mp_bitcnt_t bitsA = 100 + (i * 70);
    const mp_bitcnt_t bitsB = 80 + (i * 65);
    mpz_class a = rng.get_z_bits(bitsA);
    mpz_class b = rng.get_z_bits(bitsB);

    if (i % 2 == 1) {
      a = -a;
    }
    if (i % 3 == 2) {
      b = -b;
    }

    const mpz_class expected = a * b;
    const mpz_class actual = toom3(a, b);
    ASSERT_EQ(actual, expected);
  }
}

TEST_CASE(Toom3, ParallelMultiplication) {
  gmp_randclass rng(gmp_randinit_mt);
  rng.seed(12345UL);

  for (std::size_t limbs : {128, 256, 512}) {
    const mp_bitcnt_t bits = static_cast<mp_bitcnt_t>(limbs) * GMP_NUMB_BITS;
    mpz_class x = rng.get_z_bits(bits);
    mpz_class y = rng.get_z_bits(bits);
    mpz_setbit(x.get_mpz_t(), bits - 1);
    mpz_setbit(y.get_mpz_t(), bits - 1);

    const mpz_class expected = x * y;
    const mpz_class actualParallel = toom3_parallel(x, y, DEFAULT_CUTOFF, DEFAULT_TOOM_CUTOFF, 64);
    ASSERT_EQ(actualParallel, expected);
  }
}

TEST_CASE(Toom3, InPlaceAliasing) {
  gmp_randclass rng(gmp_randinit_mt);
  rng.seed(7654UL);

  for (std::size_t limbs : {32, 80, 160}) {
    const mp_bitcnt_t bits = static_cast<mp_bitcnt_t>(limbs) * GMP_NUMB_BITS;
    mpz_class a = rng.get_z_bits(bits);
    mpz_class b = rng.get_z_bits(bits);
    const mpz_class expected = a * b;

    mpz_class resA = a;
    toom3_impl(resA.get_mpz_t(), resA.get_mpz_t(), b.get_mpz_t(), DEFAULT_CUTOFF,
               DEFAULT_TOOM_CUTOFF, 0, 0);
    ASSERT_EQ(resA, expected);

    mpz_class resB = b;
    toom3_impl(resB.get_mpz_t(), a.get_mpz_t(), resB.get_mpz_t(), DEFAULT_CUTOFF,
               DEFAULT_TOOM_CUTOFF, 0, 0);
    ASSERT_EQ(resB, expected);

    // Square in place
    mpz_class resSq = a;
    toom3_impl(resSq.get_mpz_t(), resSq.get_mpz_t(), resSq.get_mpz_t(), DEFAULT_CUTOFF,
               DEFAULT_TOOM_CUTOFF, 0, 0);
    ASSERT_EQ(resSq, a * a);
  }
}

TEST_CASE(Toom3, ExtremeAsymmetryStress) {
  gmp_randclass rng(gmp_randinit_mt);
  rng.seed(8888UL);

  for (std::size_t smallLimbs : {1, 35, 65, 75, 120}) {
    const mpz_class large = rng.get_z_bits(1200 * GMP_NUMB_BITS);
    const mpz_class small = rng.get_z_bits(smallLimbs * GMP_NUMB_BITS);
    const mpz_class expected = large * small;

    ASSERT_EQ(toom3(large, small, DEFAULT_CUTOFF, DEFAULT_TOOM_CUTOFF), expected);
    ASSERT_EQ(toom3(small, large, DEFAULT_CUTOFF, DEFAULT_TOOM_CUTOFF), expected);
  }
}
