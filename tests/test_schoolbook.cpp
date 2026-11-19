#include <gmpxx.h>

#include "karatsuba/schoolbook.hpp"
#include "test_framework.hpp"

using namespace km;

TEST_CASE(Schoolbook, ZeroOperands) {
  ASSERT_EQ(schoolbook(0, 0), 0);
  ASSERT_EQ(schoolbook(0, 42), 0);
  ASSERT_EQ(schoolbook(42, 0), 0);
  ASSERT_EQ(schoolbook(0, -9999), 0);
  ASSERT_EQ(schoolbook(-9999, 0), 0);
}

TEST_CASE(Schoolbook, IdentityAndSmallOperands) {
  const mpz_class val("12345678901234567890");
  ASSERT_EQ(schoolbook(val, 1), val);
  ASSERT_EQ(schoolbook(1, val), val);
  ASSERT_EQ(schoolbook(val, -1), -val);
  ASSERT_EQ(schoolbook(-1, val), -val);
  ASSERT_EQ(schoolbook(-val, -1), val);

  ASSERT_EQ(schoolbook(6, 7), 42);
  ASSERT_EQ(schoolbook(12, 12), 144);
}

TEST_CASE(Schoolbook, SignCombinations) {
  const mpz_class a("987654321987654321");
  const mpz_class b("123456789123456789");
  const mpz_class expected = a * b;

  ASSERT_EQ(schoolbook(a, b), expected);
  ASSERT_EQ(schoolbook(-a, b), -expected);
  ASSERT_EQ(schoolbook(a, -b), -expected);
  ASSERT_EQ(schoolbook(-a, -b), expected);
}

TEST_CASE(Schoolbook, AsymmetricOperandSizes) {
  const mpz_class small(42);
  mpz_class large("1");
  large <<= 500;  // 500 bits
  large += 1337;

  ASSERT_EQ(schoolbook(small, large), small * large);
  ASSERT_EQ(schoolbook(large, small), large * small);
}

TEST_CASE(Schoolbook, RandomizedStress) {
  gmp_randclass rng(gmp_randinit_mt);
  rng.seed(12345UL);

  for (std::size_t i = 0; i < 50; ++i) {
    const mp_bitcnt_t bitsA = 50 + (i * 30);
    const mp_bitcnt_t bitsB = 40 + (i * 25);
    mpz_class a = rng.get_z_bits(bitsA);
    mpz_class b = rng.get_z_bits(bitsB);

    if (i % 2 == 1)
      a = -a;
    if (i % 3 == 1)
      b = -b;

    const mpz_class expected = a * b;
    const mpz_class actual = schoolbook(a, b);
    ASSERT_EQ(actual, expected);
  }
}

TEST_CASE(Schoolbook, InPlaceAliasing) {
  gmp_randclass rng(gmp_randinit_mt);
  rng.seed(4242UL);

  for (std::size_t bits : {64, 256, 1024}) {
    mpz_class a = rng.get_z_bits(bits);
    mpz_class b = rng.get_z_bits(bits);
    const mpz_class expected = a * b;

    mpz_class resA = a;
    schoolbook_impl(resA.get_mpz_t(), resA.get_mpz_t(), b.get_mpz_t());
    ASSERT_EQ(resA, expected);

    mpz_class resB = b;
    schoolbook_impl(resB.get_mpz_t(), a.get_mpz_t(), resB.get_mpz_t());
    ASSERT_EQ(resB, expected);

    // Square in place
    mpz_class resSq = a;
    schoolbook_impl(resSq.get_mpz_t(), resSq.get_mpz_t(), resSq.get_mpz_t());
    ASSERT_EQ(resSq, a * a);
  }
}
