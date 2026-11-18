#include "karatsuba/parser.hpp"

#include <cctype>
#include <string>

namespace km {

bool parseInteger(std::string_view input, mpz_class& output, int base) {
  // Trim leading/trailing whitespace
  while (!input.empty() && std::isspace(static_cast<unsigned char>(input.front()))) {
    input.remove_prefix(1);
  }
  while (!input.empty() && std::isspace(static_cast<unsigned char>(input.back()))) {
    input.remove_suffix(1);
  }
  if (input.empty()) {
    return false;
  }

  const bool negative = (input.front() == '-');
  if (negative || input.front() == '+') {
    input.remove_prefix(1);
  }

  // Detect or validate prefix and base
  int effectiveBase = base;
  if (input.starts_with("0b") || input.starts_with("0B")) {
    if (base != 0 && base != 2) {
      return false;
    }
    effectiveBase = 2;
    input.remove_prefix(2);
  } else if (input.starts_with("0x") || input.starts_with("0X")) {
    if (base != 0 && base != 16) {
      return false;
    }
    effectiveBase = 16;
    input.remove_prefix(2);
  } else if (effectiveBase == 0) {
    effectiveBase = 10;
  }

  if (input.empty()) {
    return false;
  }

  // Validate characters based on effectiveBase
  auto isValidDigit = [effectiveBase](char c) -> bool {
    if (effectiveBase == 2) {
      return c == '0' || c == '1';
    }
    if (effectiveBase == 8) {
      return c >= '0' && c <= '7';
    }
    if (effectiveBase == 10) {
      return c >= '0' && c <= '9';
    }
    if (effectiveBase == 16) {
      return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }
    return false;
  };

  for (char c : input) {
    if (!isValidDigit(c)) {
      return false;
    }
  }

  std::string cleanStr(input);
  if (mpz_set_str(output.get_mpz_t(), cleanStr.c_str(), effectiveBase) != 0) {
    return false;
  }

  if (negative && output != 0) {
    output = -output;
  }
  return true;
}

std::string formatInteger(const mpz_class& value, int base, bool includePrefix) {
  std::string str = value.get_str(base);
  if (!includePrefix) {
    return str;
  }

  const bool isNegative = (str.front() == '-');
  std::string prefix;
  if (base == 2) {
    prefix = "0b";
  } else if (base == 16) {
    prefix = "0x";
  } else if (base == 8) {
    prefix = "0o";
  }

  if (prefix.empty()) {
    return str;
  }

  if (isNegative) {
    return "-" + prefix + str.substr(1);
  }
  return prefix + str;
}

}  // namespace km
