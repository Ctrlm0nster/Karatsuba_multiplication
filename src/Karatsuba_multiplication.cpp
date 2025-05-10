#include "Karatsuba_multiplication.hpp"

int Karatsuba_base2_multiplication::makeEqualLength(std::string &str1,
                                                    std::string &str2) {
  int len1 = static_cast<int>(str1.size());
  int len2 = static_cast<int>(str2.size());

  if (len1 < len2) {
    for (int i = 0; i < (len2 - len1); i++) {
      str1 = '0' + str1;
    }
    return len2;
  } else if (len1 > len2) {
    for (int i = 0; i < (len1 - len2); i++) {
      str2 = '0' + str2;
    }
    return len1;
  }
  return len1; // If len1 >= len2
}

std::string Karatsuba_base2_multiplication::addBitString(std::string first,
                                                         std::string second) {
  std::string result; // To store the sum bit

  int lenght = makeEqualLength(first, second);
  int carry = 0;

  // Add all the bits one by one
  for (int i = lenght - 1; i >= 0; i--) {
    int firstBit = first.at(i) - '0';
    int secondBit = second.at(i) - '0';

    // Boolean expression for sum of three bits
    int sum = (firstBit ^ secondBit ^ carry) + '0';

    result = static_cast<char>(sum) + result;

    // Boolean expression for 3-bit addition
    carry = (firstBit & secondBit) | (secondBit & carry) | (firstBit & carry);
  }

  // if overflow, the add a leading 1
  if (carry) {
    result = '1' + result;
  }
  return result;
}

int Karatsuba_base2_multiplication::multiplySingleBit(std::string a,
                                                      std::string b) {
  return (a[0] - '0') * (b[0] - '0');
}

long int Karatsuba_base2_multiplication::multiply(std::string X,
                                                  std::string Y) {
  // Find the maximum of lengths of x and Y and make length
  // of smaller string same as that of larger string
  Karatsuba_base2_multiplication karatsuba;
  int n = karatsuba.makeEqualLength(X, Y);

  // Base case
  if (n == 0)
    return 0;
  if (n == 1)
    return karatsuba.multiplySingleBit(X, Y);

  int firstHalf = n / 2;            // First half of the string, floor(n/2)
  int secondHalf = (n - firstHalf); // Second half of the string, ceil (n/2)

  // Find the first and second half of the first string
  std::string xf = X.substr(0, firstHalf);
  std::string xs = X.substr(firstHalf, secondHalf);

  // Find the first and second half of the second string
  std::string yf = Y.substr(0, firstHalf);
  std::string ys = Y.substr(firstHalf, secondHalf);

  // Recursively calculate the three product of the inputs of size n/2
  long int P1 = multiply(xf, yf);
  long int P2 = multiply(xs, ys);
  long int P3 =
      multiply(karatsuba.addBitString(xf, xs), karatsuba.addBitString(yf, ys));

  // Combine the three product to get the final result
  return P1 * (1 << (2 * secondHalf)) + (P3 - P1 - P2) * (1 << secondHalf) + P2;
}
