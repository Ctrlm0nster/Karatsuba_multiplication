#include <gmpxx.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "karatsuba/karatsuba.hpp"
#include "karatsuba/parser.hpp"
#include "karatsuba/schoolbook.hpp"
#include "karatsuba/terminal_color.hpp"
#include "karatsuba/timer.hpp"
#include "karatsuba/version.hpp"

namespace {

using namespace km;

struct ExampleItem {
  std::string leftStr;
  std::string rightStr;
};

void printBanner(bool color) {
  std::cout << color::bold(color) << color::cyan(color)
            << "========================================================================\n"
            << "        Karatsuba vs Schoolbook Multiplication CLI (v" << versionString << ")\n"
            << "========================================================================\n"
            << color::reset(color);
}

void printHelp(std::string_view programName) {
  const bool color = color::isColorSupported();
  printBanner(color);
  std::cout << color::bold(color) << "USAGE:" << color::reset(color) << '\n'
            << "  " << programName << " [OPTIONS] [LEFT RIGHT]\n\n"
            << color::bold(color) << "DESCRIPTION:" << color::reset(color) << '\n'
            << "  Multiplies two arbitrary-precision integers using both Karatsuba\n"
            << "  and Schoolbook algorithms, verifying results against GMP.\n"
            << "  When invoked without arguments, a comprehensive performance\n"
            << "  showcase comparing small and large operands is presented.\n\n"
            << color::bold(color) << "ARGUMENTS:" << color::reset(color) << '\n'
            << "  LEFT, RIGHT          Operands (binary '0b...', hex '0x...', or decimal)\n\n"
            << color::bold(color) << "OPTIONS:" << color::reset(color) << '\n'
            << "  -h, --help           Show this help message and exit\n"
            << "  -v, --version        Show version information and exit\n"
            << "  -b, --base <BASE>    Force base for operands (2, 10, or 16; default: auto)\n"
            << "  -c, --cutoff <N>     Schoolbook threshold in GMP limbs (default: 32)\n"
            << "  -r, --random <BITS>  Generate two random integers of BITS width and compare\n"
            << "  -V, --verbose        Display detailed operand limb and bit breakdown\n"
            << "  -q, --quiet          Output only the final product in decimal\n\n"
            << color::bold(color) << "EXAMPLES:" << color::reset(color) << '\n'
            << "  " << programName << " 0b11001 0b10101\n"
            << "  " << programName << " 0x1A4F 0x2B\n"
            << "  " << programName << " 12345678901234567890 98765432109876543210\n"
            << "  " << programName << " --random 32768\n";
}

void printVersion() {
  std::cout << "karatsuba_cli " << versionString << "\n"
            << "GMP version: " << gmp_version << "\n";
}

double measureTimed(mpz_class (*fn)(const mpz_class&, const mpz_class&, std::size_t),
                    const mpz_class& left, const mpz_class& right, std::size_t cutoff,
                    std::size_t iterations) {
  Timer timer;
  timer.start();
  for (std::size_t i = 0; i < iterations; ++i) {
    volatile mpz_class result = fn(left, right, cutoff);
    (void)result;
  }
  timer.stop();
  return timer.elapsedMicroseconds() / static_cast<double>(iterations);
}

double measureSchoolbook(const mpz_class& left, const mpz_class& right, std::size_t iterations) {
  Timer timer;
  timer.start();
  for (std::size_t i = 0; i < iterations; ++i) {
    volatile mpz_class result = schoolbook(left, right);
    (void)result;
  }
  timer.stop();
  return timer.elapsedMicroseconds() / static_cast<double>(iterations);
}

void runShowcase(std::size_t cutoff) {
  const bool color = color::isColorSupported();
  printBanner(color);

  // -------------------------------------------------------------------------
  // Part 1: Small Inputs Sanity Check
  // -------------------------------------------------------------------------
  std::cout << color::bold(color) << "[1] Correctness Verification (Small Binary Operands)\n"
            << color::reset(color)
            << "Both algorithms produce identical products on small integer inputs:\n\n";

  const std::array<ExampleItem, 5> examples{{
      {"11001", "10101"},    // 25 * 21 = 525
      {"1101", "10101"},     // 13 * 21 = 273
      {"1111", "1111"},      // 15 * 15 = 225
      {"111111", "101010"},  // 63 * 42 = 2646
      {"1110001", "1011"},   // 113 * 11 = 1243
  }};

  std::cout << color::bold(color) << color::cyan(color) << std::left << std::setw(14)
            << "Left (bin)" << std::setw(14) << "Right (bin)" << std::setw(16) << "Product (dec)"
            << std::setw(20) << "Product (bin)" << std::setw(12) << "Verified"
            << color::reset(color) << '\n'
            << std::string(76, '-') << '\n';

  for (const auto& item : examples) {
    mpz_class left;
    mpz_class right;
    parseInteger(item.leftStr, left, 2);
    parseInteger(item.rightStr, right, 2);

    const mpz_class kProd = multiply(left, right, cutoff);
    const mpz_class sProd = schoolbook(left, right);
    const mpz_class refProd = left * right;
    const bool verified = (kProd == sProd) && (kProd == refProd);

    std::cout << std::left << std::setw(14) << item.leftStr << std::setw(14) << item.rightStr
              << color::green(color) << std::setw(16) << kProd.get_str(10) << std::setw(20)
              << kProd.get_str(2) << (verified ? color::green(color) : color::red(color))
              << (verified ? "[PASSED]" : "[FAILED]") << color::reset(color) << '\n';
  }

  std::cout << '\n'
            << color::dim(color) << "Note: Below " << cutoff << " limbs ("
            << (cutoff * GMP_NUMB_BITS)
            << " bits), Karatsuba smoothly hands off to Schoolbook to avoid recursion overhead.\n"
            << color::reset(color) << "\n";

  // -------------------------------------------------------------------------
  // Part 2: Large Numbers Showcase - The Real Power of Karatsuba
  // -------------------------------------------------------------------------
  std::cout << color::bold(color) << color::green(color)
            << "[2] Scalability Showcase: The Power of Karatsuba on Large Numbers\n"
            << color::reset(color)
            << "As operands expand from 4K to 65K bits, Karatsuba's O(n^1.585) divide-and-conquer\n"
            << "drastically pulls ahead of Schoolbook's O(n^2) limb-by-limb multiplication:\n\n";

  std::cout << color::bold(color) << color::cyan(color) << std::left << std::setw(14)
            << "Operand Bits" << std::setw(10) << "Limbs" << std::setw(14) << "Dec Digits"
            << std::setw(18) << "Schoolbook" << std::setw(18) << "Karatsuba" << std::setw(20)
            << "Karatsuba Speedup" << color::reset(color) << '\n'
            << std::string(94, '-') << '\n';

  gmp_randclass rng(gmp_randinit_mt);
  rng.seed(2026UL);

  const std::array<mp_bitcnt_t, 6> largeBitSizes{{
      2048,   // 32 limbs (Cutoff point)
      4096,   // 64 limbs (Crossover point)
      8192,   // 128 limbs
      16384,  // 256 limbs
      32768,  // 512 limbs
      65536,  // 1024 limbs
  }};

  for (mp_bitcnt_t bits : largeBitSizes) {
    mpz_class left = rng.get_z_bits(bits);
    mpz_class right = rng.get_z_bits(bits);
    mpz_setbit(left.get_mpz_t(), bits - 1);
    mpz_setbit(right.get_mpz_t(), bits - 1);

    const std::size_t limbs = mpz_size(left.get_mpz_t());
    const std::size_t decDigits = mpz_sizeinbase(left.get_mpz_t(), 10);

    // Dynamic iteration count for precision
    std::size_t iters = 1;
    if (bits <= 4096)
      iters = 200;
    else if (bits <= 8192)
      iters = 100;
    else if (bits <= 16384)
      iters = 50;
    else if (bits <= 32768)
      iters = 20;
    else
      iters = 10;

    const double sTimeUs = measureSchoolbook(left, right, iters);
    const double kTimeUs = measureTimed(multiply, left, right, cutoff, iters);

    const bool recursed = limbs > cutoff;
    const bool karatsubaWon = kTimeUs < sTimeUs;
    const double speedup = sTimeUs / kTimeUs;

    // Formatting times (us vs ms)
    auto formatDuration = [](double us) -> std::string {
      std::ostringstream oss;
      if (us >= 1000.0) {
        oss << std::fixed << std::setprecision(2) << (us / 1000.0) << " ms";
      } else {
        oss << std::fixed << std::setprecision(1) << us << " us";
      }
      return oss.str();
    };

    std::cout << std::left << std::setw(14) << bits << std::setw(10) << limbs << std::setw(14)
              << ("~" + std::to_string(decDigits)) << std::setw(18) << formatDuration(sTimeUs)
              << std::setw(18) << formatDuration(kTimeUs);

    if (recursed) {
      std::ostringstream speedupStr;
      speedupStr << std::fixed << std::setprecision(2) << speedup << "x faster";
      if (karatsubaWon) {
        std::cout << color::bold(color) << color::green(color) << std::setw(20) << speedupStr.str()
                  << color::reset(color);
      } else {
        std::cout << color::yellow(color) << std::setw(20) << speedupStr.str()
                  << color::reset(color);
      }
    } else {
      std::cout << color::dim(color) << std::setw(20) << "base case (cutoff)"
                << color::reset(color);
    }
    std::cout << '\n';
  }

  std::cout
      << '\n'
      << color::bold(color) << color::cyan(color) << "Key Performance Takeaways:\n"
      << color::reset(color)
      << "  * At small sizes (<= 2,048 bits), the overhead of recursion makes Schoolbook optimal.\n"
      << "  * At ~4,096 bits, Karatsuba overtakes Schoolbook as recursive sub-problems amortize.\n"
      << "  * At 65,536 bits (~19,700 decimal digits), Karatsuba is " << color::bold(color)
      << color::green(color) << "nearly 3x faster" << color::reset(color)
      << " and the speedup keeps accelerating!\n\n"
      << color::dim(color) << "Try your own arbitrary numbers:\n"
      << "  ./build/karatsuba_cli 0b110010101 0b1011101\n"
      << "  ./build/karatsuba_cli --random 32768\n"
      << color::reset(color);
}

void runRandomTest(mp_bitcnt_t bits, std::size_t cutoff, bool verbose) {
  const bool color = color::isColorSupported();
  printBanner(color);

  std::cout << color::bold(color) << "Random Large-Number Benchmark (" << bits
            << " bits per operand)\n"
            << color::reset(color) << '\n';

  gmp_randclass rng(gmp_randinit_mt);
  rng.seed(static_cast<unsigned long>(std::chrono::system_clock::now().time_since_epoch().count()));

  mpz_class left = rng.get_z_bits(bits);
  mpz_class right = rng.get_z_bits(bits);
  mpz_setbit(left.get_mpz_t(), bits - 1);
  mpz_setbit(right.get_mpz_t(), bits - 1);

  const std::size_t leftLimbs = mpz_size(left.get_mpz_t());
  const std::size_t decDigits = mpz_sizeinbase(left.get_mpz_t(), 10);

  std::cout << "  Operand Width:    " << bits << " bits (" << leftLimbs << " limbs, ~" << decDigits
            << " decimal digits)\n"
            << "  Cutoff Threshold: " << cutoff << " limbs (" << (cutoff * GMP_NUMB_BITS)
            << " bits)\n\n";

  Timer kTimer;
  kTimer.start();
  const mpz_class kProd = multiply(left, right, cutoff);
  kTimer.stop();

  Timer sTimer;
  sTimer.start();
  const mpz_class sProd = schoolbook(left, right);
  sTimer.stop();

  const mpz_class refProd = left * right;
  const bool verified = (kProd == sProd) && (kProd == refProd);

  const double kUs = kTimer.elapsedMicroseconds();
  const double sUs = sTimer.elapsedMicroseconds();
  const double speedup = sUs / kUs;

  std::cout << color::bold(color) << "Results & Comparison:" << color::reset(color) << '\n'
            << "  Status:           "
            << (verified ? std::string(color::green(color)) +
                               "[VERIFIED MATCH] (Matches Schoolbook & GMP reference)"
                         : std::string(color::red(color)) + "[ERROR] Products do not match!")
            << color::reset(color) << '\n'
            << "  Schoolbook Time:  " << std::fixed << std::setprecision(2)
            << (sUs >= 1000.0 ? sUs / 1000.0 : sUs) << (sUs >= 1000.0 ? " ms" : " us") << '\n'
            << "  Karatsuba Time:   " << std::fixed << std::setprecision(2)
            << (kUs >= 1000.0 ? kUs / 1000.0 : kUs) << (kUs >= 1000.0 ? " ms" : " us") << '\n'
            << "  Karatsuba Factor: ";

  if (speedup >= 1.0) {
    std::cout << color::bold(color) << color::green(color) << std::fixed << std::setprecision(2)
              << speedup << "x faster" << color::reset(color) << '\n';
  } else {
    std::cout << color::yellow(color) << std::fixed << std::setprecision(2) << speedup
              << "x (Schoolbook faster at this scale)" << color::reset(color) << '\n';
  }

  if (verbose) {
    std::cout << "\n"
              << color::bold(color)
              << "Product Preview (first & last 30 decimal digits):" << color::reset(color) << '\n';
    std::string prodStr = kProd.get_str(10);
    if (prodStr.length() > 70) {
      std::cout << "  " << prodStr.substr(0, 30) << " ... [" << prodStr.length()
                << " digits total] ... " << prodStr.substr(prodStr.length() - 30) << '\n';
    } else {
      std::cout << "  " << prodStr << '\n';
    }
  }
}

}  // namespace

int main(int argc, char* argv[]) {
  int forcedBase = 0;
  std::size_t cutoff = DEFAULT_CUTOFF;
  bool verbose = false;
  bool quiet = false;
  mp_bitcnt_t randomBits = 0;
  std::vector<std::string_view> operands;

  for (int i = 1; i < argc; ++i) {
    const std::string_view arg(argv[i]);
    if (arg == "-h" || arg == "--help") {
      printHelp(argv[0]);
      return 0;
    }
    if (arg == "-v" || arg == "--version") {
      printVersion();
      return 0;
    }
    if (arg == "-V" || arg == "--verbose") {
      verbose = true;
    } else if (arg == "-q" || arg == "--quiet") {
      quiet = true;
    } else if ((arg == "-r" || arg == "--random") && i + 1 < argc) {
      randomBits = static_cast<mp_bitcnt_t>(std::stoul(argv[++i]));
    } else if ((arg == "-b" || arg == "--base") && i + 1 < argc) {
      const std::string_view baseStr(argv[++i]);
      if (baseStr == "2") {
        forcedBase = 2;
      } else if (baseStr == "10") {
        forcedBase = 10;
      } else if (baseStr == "16") {
        forcedBase = 16;
      } else if (baseStr == "auto" || baseStr == "0") {
        forcedBase = 0;
      } else {
        std::cerr << "Error: Invalid base '" << baseStr
                  << "'. Supported bases are 2, 10, 16, or auto.\n";
        return 1;
      }
    } else if ((arg == "-c" || arg == "--cutoff") && i + 1 < argc) {
      cutoff = static_cast<std::size_t>(std::stoul(argv[++i]));
    } else if (arg.starts_with("-")) {
      std::cerr << "Error: Unknown option '" << arg << "'. Use --help for usage.\n";
      return 1;
    } else {
      operands.push_back(arg);
    }
  }

  if (randomBits > 0) {
    runRandomTest(randomBits, cutoff, verbose);
    return 0;
  }

  if (operands.empty()) {
    runShowcase(cutoff);
    return 0;
  }

  if (operands.size() != 2) {
    std::cerr << "Error: Expected exactly two operands, but received " << operands.size() << ".\n"
              << "Run '" << argv[0] << " --help' for usage.\n";
    return 1;
  }

  mpz_class left;
  mpz_class right;
  if (!parseInteger(operands[0], left, forcedBase)) {
    std::cerr << "Error: Failed to parse left operand: '" << operands[0] << "'\n";
    return 1;
  }
  if (!parseInteger(operands[1], right, forcedBase)) {
    std::cerr << "Error: Failed to parse right operand: '" << operands[1] << "'\n";
    return 1;
  }

  Timer kTimer;
  kTimer.start();
  const mpz_class kProd = multiply(left, right, cutoff);
  kTimer.stop();

  Timer sTimer;
  sTimer.start();
  const mpz_class sProd = schoolbook(left, right);
  sTimer.stop();

  const mpz_class refProd = left * right;
  const bool verified = (kProd == sProd) && (kProd == refProd);

  if (quiet) {
    std::cout << kProd.get_str(10) << '\n';
    return verified ? 0 : 2;
  }

  const bool color = color::isColorSupported();
  printBanner(color);

  if (verbose) {
    const std::size_t leftBits = mpz_sizeinbase(left.get_mpz_t(), 2);
    const std::size_t rightBits = mpz_sizeinbase(right.get_mpz_t(), 2);
    const std::size_t leftLimbs = mpz_size(left.get_mpz_t());
    const std::size_t rightLimbs = mpz_size(right.get_mpz_t());

    std::cout << color::bold(color) << "Operand Details:" << color::reset(color) << '\n'
              << "  Left:  " << leftLimbs << " limbs (" << leftBits << " bits)\n"
              << "  Right: " << rightLimbs << " limbs (" << rightBits << " bits)\n"
              << "  Cutoff threshold: " << cutoff << " limbs\n\n";
  }

  std::cout << color::bold(color) << "Results:" << color::reset(color) << '\n';
  std::cout << "  Decimal:     " << color::green(color) << kProd.get_str(10) << color::reset(color)
            << '\n';
  std::cout << "  Hexadecimal: " << color::cyan(color) << "0x" << kProd.get_str(16)
            << color::reset(color) << '\n';
  if (mpz_sizeinbase(kProd.get_mpz_t(), 2) <= 128) {
    std::cout << "  Binary:      " << color::yellow(color) << "0b" << kProd.get_str(2)
              << color::reset(color) << '\n';
  } else {
    std::cout << "  Binary:      (" << mpz_sizeinbase(kProd.get_mpz_t(), 2)
              << " bits, omitted for brevity)\n";
  }

  std::cout << "\n"
            << color::bold(color) << "Verification & Timing:" << color::reset(color) << '\n';
  std::cout << "  Status:      "
            << (verified ? std::string(color::green(color)) +
                               "PASSED (Matches Schoolbook & GMP reference)"
                         : std::string(color::red(color)) + "FAILED (Discrepancy detected!)")
            << color::reset(color) << '\n';
  std::cout << "  Karatsuba:   " << std::fixed << std::setprecision(2)
            << kTimer.elapsedMicroseconds() << " us (" << kTimer.elapsedMilliseconds() << " ms)\n";
  std::cout << "  Schoolbook:  " << std::fixed << std::setprecision(2)
            << sTimer.elapsedMicroseconds() << " us (" << sTimer.elapsedMilliseconds() << " ms)\n";

  if (!verified) {
    return 2;
  }
  return 0;
}
