#include "Binary_multiplication.hpp"

static int binaryToDecimal(const std::string& binary) {
  int decimal = 0;
  for (char bit : binary) {
    decimal = decimal * 2 + (bit - '0');
  }
  return decimal;
}

std::string decimalToBinary(int decimal) {
  if (decimal == 0)
    return "0";
  std::string binary;
  while (decimal > 0) {
    binary = std::to_string(decimal % 2) + binary;
    decimal /= 2;
  }
  return binary;
}

std::string binaryMultiplication(const std::string& a, const std::string& b) {
  // Convert binary strings to decimal integers
  int numA = binaryToDecimal(a);
  int numB = binaryToDecimal(b);

  // Perform multiplication
  int product = numA * numB;

  // Convert the product back to a binary string
  return decimalToBinary(product);
}
