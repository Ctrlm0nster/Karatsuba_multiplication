#include "karatsuba/parser.hpp"
#include "test_framework.hpp"

using namespace km;

TEST_CASE(Parser, BinaryInputs) {
  mpz_class val;
  ASSERT_TRUE(parseInteger("0b1010", val));
  ASSERT_EQ(val, 10);

  ASSERT_TRUE(parseInteger("0B1111", val));
  ASSERT_EQ(val, 15);

  ASSERT_TRUE(parseInteger("-0b101", val));
  ASSERT_EQ(val, -5);

  ASSERT_TRUE(parseInteger("+0b11001", val));
  ASSERT_EQ(val, 25);

  // Forced base 2 without prefix
  ASSERT_TRUE(parseInteger("1101", val, 2));
  ASSERT_EQ(val, 13);
}

TEST_CASE(Parser, HexInputs) {
  mpz_class val;
  ASSERT_TRUE(parseInteger("0x1A", val));
  ASSERT_EQ(val, 26);

  ASSERT_TRUE(parseInteger("0XFF", val));
  ASSERT_EQ(val, 255);

  ASSERT_TRUE(parseInteger("-0x2B", val));
  ASSERT_EQ(val, -43);

  // Forced base 16 without prefix
  ASSERT_TRUE(parseInteger("deadbeef", val, 16));
  ASSERT_EQ(val, mpz_class("3735928559"));
}

TEST_CASE(Parser, DecimalInputs) {
  mpz_class val;
  ASSERT_TRUE(parseInteger("12345678901234567890", val));
  ASSERT_EQ(val, mpz_class("12345678901234567890"));

  ASSERT_TRUE(parseInteger("-987654321", val));
  ASSERT_EQ(val, -987654321);

  ASSERT_TRUE(parseInteger("+42", val));
  ASSERT_EQ(val, 42);
}

TEST_CASE(Parser, WhitespaceAndTrimming) {
  mpz_class val;
  ASSERT_TRUE(parseInteger("   0b101   ", val));
  ASSERT_EQ(val, 5);

  ASSERT_TRUE(parseInteger("\t\n0xFF\r\n", val));
  ASSERT_EQ(val, 255);
}

TEST_CASE(Parser, InvalidInputs) {
  mpz_class val;
  ASSERT_FALSE(parseInteger("", val));
  ASSERT_FALSE(parseInteger("    ", val));
  ASSERT_FALSE(parseInteger("0b", val));
  ASSERT_FALSE(parseInteger("0x", val));
  ASSERT_FALSE(parseInteger("0b102", val));
  ASSERT_FALSE(parseInteger("0x12G", val));
  ASSERT_FALSE(parseInteger("123a456", val));
  ASSERT_FALSE(parseInteger("++5", val));
  ASSERT_FALSE(parseInteger("--5", val));
  ASSERT_FALSE(parseInteger("-+5", val));
  ASSERT_FALSE(parseInteger("0b101", val, 10));  // Base mismatch
}

TEST_CASE(Parser, FormatInteger) {
  const mpz_class val(42);
  ASSERT_EQ(formatInteger(val, 10), "42");
  ASSERT_EQ(formatInteger(val, 2), "101010");
  ASSERT_EQ(formatInteger(val, 2, true), "0b101010");
  ASSERT_EQ(formatInteger(val, 16), "2a");
  ASSERT_EQ(formatInteger(val, 16, true), "0x2a");

  const mpz_class neg(-42);
  ASSERT_EQ(formatInteger(neg, 10), "-42");
  ASSERT_EQ(formatInteger(neg, 2, true), "-0b101010");
  ASSERT_EQ(formatInteger(neg, 16, true), "-0x2a");
}
