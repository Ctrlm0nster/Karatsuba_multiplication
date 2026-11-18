// Compatibility entry point - see apps/karatsuba_cli.cpp for the primary CLI
// application
#include <iostream>

#include "karatsuba/karatsuba.hpp"
#include "karatsuba/parser.hpp"
#include "karatsuba/schoolbook.hpp"
#include "karatsuba/terminal_color.hpp"
#include "karatsuba/timer.hpp"

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cout << "Usage: " << argv[0] << " <left> <right>\n"
              << "For full features, use the karatsuba_cli application.\n";
    return 1;
  }

  mpz_class left;
  mpz_class right;
  if (!karatsuba::parseInteger(argv[1], left) || !karatsuba::parseInteger(argv[2], right)) {
    std::cerr << "Error parsing operands.\n";
    return 2;
  }

  const mpz_class kProd = karatsuba::multiply(left, right);
  const mpz_class sProd = karatsuba::schoolbook(left, right);

  std::cout << "Product (decimal): " << kProd.get_str() << '\n';
  if (kProd == sProd) {
    std::cout << "Karatsuba and schoolbook agree.\n";
    return 0;
  }
  std::cerr << "Mismatch between Karatsuba and schoolbook!\n";
  return 3;
}
