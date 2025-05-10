#pragma once
// C++ implementation of Karatsuba algorithm for bit string multiplication
#include <string>

class Karatsuba_base2_multiplication {
  // HELPER METHOD: given two unequal sized bit string, converts them to same
  // length by adding leading 0s in the smaller string, returns the new length
  int makeEqualLength(std::string &str1, std::string &str2);
  // Function that add two bit sequence and returns the addition
  std::string addBitString(std::string first, std::string second);
  // A utility function to multiply single bits of string a and b
  int multiplySingleBit(std::string a, std::string b);

public:
  // The main function that multiplies two bit strings X and Y and returns
  // result as long integer.
  static long int multiply(std::string X, std::string Y);
};
