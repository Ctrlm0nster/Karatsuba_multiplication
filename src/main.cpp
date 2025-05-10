// STL headers
#include <iostream>
#include <string>
// User-defined headers
#include "Binary_multiplication.hpp"
#include "Karatsuba_multiplication.hpp"
#include "timer.hpp"
int main() {
  Timer timer;
  timer.Start();
  std::cout << "11001 * 10101 = "
    << Karatsuba_base2_multiplication::multiply("11001", "10101") << '\n';
  std::cout << "1101 * 10101 = "
    << Karatsuba_base2_multiplication::multiply("1101", "10101") << '\n';
  std::cout << "111 * 10101 = "
    << Karatsuba_base2_multiplication::multiply("111", "10101") << '\n';
  std::cout << "11 * 10101 = "
    << Karatsuba_base2_multiplication::multiply("11", "10101") << '\n';
  std::cout << "01 * 10101 = "
    << Karatsuba_base2_multiplication::multiply("01", "10101") << '\n';
  std::cout << "1111 * 1111 = "
    << Karatsuba_base2_multiplication::multiply("1111", "1111") << '\n';
  std::cout << "111 * 1111 = "
    << Karatsuba_base2_multiplication::multiply("111", "1111") << '\n';
  timer.Stop();
  timer.DisplayTime();

  Timer timer2;
  timer2.Start();
  std::cout << "11001 * 10101 = " << binaryMultiplication("11001", "10101") << '\n';
  std::cout << "1101 * 10101 = " << binaryMultiplication("1101", "10101") << '\n';
  std::cout << "111 * 10101 = " << binaryMultiplication("111", "10101") << '\n';
  std::cout << "11 * 10101 = " << binaryMultiplication("11", "10101") << '\n';
  std::cout << "01 * 10101 = " << binaryMultiplication("01", "10101") << '\n';
  std::cout << "1111 * 1111 = " << binaryMultiplication("1111", "1111") << '\n';
  std::cout << "111 * 1111 = " << binaryMultiplication("111", "1111") << '\n';
  timer2.Stop();
  timer2.DisplayTime();
  return 0;
}
