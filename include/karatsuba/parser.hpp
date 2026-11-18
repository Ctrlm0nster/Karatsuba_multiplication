#pragma once

#include <gmpxx.h>

#include <string>
#include <string_view>

namespace km {

/**
 * @brief Parses an integer string into an mpz_class value.
 *
 * Supports optional leading '+' or '-' sign.
 * If base is 0 (auto-detect):
 *   - "0b" or "0B" prefix: parsed as binary (base 2)
 *   - "0x" or "0X" prefix: parsed as hexadecimal (base 16)
 *   - Otherwise: parsed as decimal (base 10)
 *
 * If base is 2, 10, or 16, optional prefixes ("0b", "0x") are also accepted
 * and stripped.
 *
 * @param input The input string view
 * @param output The output mpz_class
 * @param base Base to parse in (0 for auto, or 2, 10, 16)
 * @return true if successfully parsed, false otherwise
 */
bool parseInteger(std::string_view input, mpz_class& output, int base = 0);

/**
 * @brief Formats an mpz_class integer as a string in the specified base.
 *
 * @param value The integer to format
 * @param base Output base (2, 10, or 16)
 * @param includePrefix Whether to include "0b" or "0x" prefix
 * @return Formatted string
 */
std::string formatInteger(const mpz_class& value, int base = 10, bool includePrefix = false);

}  // namespace km
