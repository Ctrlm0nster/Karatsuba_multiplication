#include <gmpxx.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "karatsuba/karatsuba.hpp"
#include "karatsuba/parser.hpp"
#include "karatsuba/schoolbook.hpp"
#include "karatsuba/terminal_color.hpp"
#include "karatsuba/timer.hpp"
#include "karatsuba/toom3.hpp"
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
            << " Arbitrary-Precision Multiplication CLI: Karatsuba & Toom-Cook (v" << versionString
            << ")\n"
            << "========================================================================\n"
            << color::reset(color);
}

void printHelp(std::string_view programName) {
  const bool color = color::isColorSupported();
  printBanner(color);
  std::cout << color::bold(color) << "USAGE:" << color::reset(color) << '\n'
            << "  " << programName << " [OPTIONS] [LEFT RIGHT]\n\n"
            << color::bold(color) << "DESCRIPTION:" << color::reset(color) << '\n'
            << "  Multiplies two arbitrary-precision integers using Schoolbook, Karatsuba,\n"
            << "  and Toom-3 (Toom-Cook 3-way) algorithms, with optional multi-threading,\n"
            << "  verifying all results against GMP native arithmetic.\n"
            << "  When invoked without arguments, an interactive scalability showcase\n"
            << "  comparing all algorithms across increasing operand sizes is presented.\n\n"
            << color::bold(color) << "ARGUMENTS:" << color::reset(color) << '\n'
            << "  LEFT, RIGHT              Operands (binary '0b...', hex '0x...', or decimal)\n\n"
            << color::bold(color) << "OPTIONS:" << color::reset(color) << '\n'
            << "  -h, --help               Show this help message and exit\n"
            << "  -v, --version            Show version information and exit\n"
            << "  -b, --base <BASE>        Force base for operands (2, 10, or 16; default: auto)\n"
            << "  -c, --cutoff <N>         Karatsuba threshold in GMP limbs (default: 32)\n"
            << "  -t, --toom-cutoff <N>    Toom-3 threshold in GMP limbs (default: 64)\n"
            << "  -p, --parallel-cutoff <N> Multi-threading threshold in limbs (default: 256)\n"
            << "  --parallel               Force parallel multi-threading for the calculation\n"
            << "  -r, --random <BITS>      Generate two random integers of BITS width and compare\n"
            << "  -V, --verbose            Display detailed operand limb and bit breakdown\n"
            << "  -q, --quiet              Output only the final product in decimal\n\n"
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

template <typename F>
double measureExecution(F&& fn, std::size_t iterations) {
  Timer timer;
  timer.start();
  for (std::size_t i = 0; i < iterations; ++i) {
    volatile mpz_class result = fn();
    (void)result;
  }
  timer.stop();
  return timer.elapsedMicroseconds() / static_cast<double>(iterations);
}

std::string formatDuration(double us) {
  std::ostringstream oss;
  if (us >= 1000.0) {
    oss << std::fixed << std::setprecision(2) << (us / 1000.0) << " ms";
  } else {
    oss << std::fixed << std::setprecision(1) << us << " us";
  }
  return oss.str();
}

void runShowcase(std::size_t karaCutoff, std::size_t toomCutoff, std::size_t parCutoff) {
  const bool color = color::isColorSupported();
  printBanner(color);

  // -------------------------------------------------------------------------
  // Part 1: Small Inputs Sanity Check
  // -------------------------------------------------------------------------
  std::cout << color::bold(color) << "[1] Correctness Verification (Small Binary Operands)\n"
            << color::reset(color)
            << "All algorithms produce identical products on small integer inputs:\n\n";

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

    const mpz_class sProd = schoolbook(left, right);
    const mpz_class kProd = multiply(left, right, karaCutoff);
    const mpz_class tProd = toom3(left, right, karaCutoff, toomCutoff);
    const mpz_class refProd = left * right;
    const bool verified = (kProd == sProd) && (tProd == refProd) && (kProd == refProd);

    std::cout << std::left << std::setw(14) << item.leftStr << std::setw(14) << item.rightStr
              << color::green(color) << std::setw(16) << kProd.get_str(10) << std::setw(20)
              << kProd.get_str(2) << (verified ? color::green(color) : color::red(color))
              << (verified ? "[PASSED]" : "[FAILED]") << color::reset(color) << '\n';
  }

  std::cout << '\n'
            << color::dim(color)
            << "Note: Cutoffs are configurable (Karatsuba cutoff: " << karaCutoff
            << " limbs, Toom-3 cutoff: " << toomCutoff << " limbs, Parallel cutoff: " << parCutoff
            << " limbs).\n"
            << color::reset(color) << "\n";

  // -------------------------------------------------------------------------
  // Part 2: Large Numbers Showcase - Algorithmic Comparison
  // -------------------------------------------------------------------------
  std::cout << color::bold(color) << color::green(color)
            << "[2] Scalability Showcase: Schoolbook vs Karatsuba vs Toom-3 vs Parallel\n"
            << color::reset(color)
            << "As operands expand, higher-order divide-and-conquer and multi-threading\n"
            << "yield compounding speedups over quadratic Schoolbook multiplication:\n\n";

  std::cout << color::bold(color) << color::cyan(color) << std::left << std::setw(12) << "Bits"
            << std::setw(8) << "Limbs" << std::setw(12) << "Digits" << std::setw(15) << "Schoolbook"
            << std::setw(15) << "Karatsuba" << std::setw(15) << "Toom-3" << std::setw(16)
            << "Parallel" << std::setw(16) << "Top Speedup" << color::reset(color) << '\n'
            << std::string(109, '-') << '\n';

  gmp_randclass rng(gmp_randinit_mt);
  rng.seed(2026UL);

  const std::array<mp_bitcnt_t, 6> largeBitSizes{{
      2048,   // 32 limbs
      4096,   // 64 limbs
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

    std::size_t iters = 1;
    if (bits <= 4096) {
      iters = 100;
    } else if (bits <= 8192) {
      iters = 50;
    } else if (bits <= 16384) {
      iters = 25;
    } else if (bits <= 32768) {
      iters = 10;
    } else {
      iters = 5;
    }

    const double sTimeUs = measureExecution([&]() { return schoolbook(left, right); }, iters);
    const double kTimeUs =
        measureExecution([&]() { return multiply(left, right, karaCutoff); }, iters);
    const double tTimeUs =
        measureExecution([&]() { return toom3(left, right, karaCutoff, toomCutoff); }, iters);
    const double pTimeUs = measureExecution(
        [&]() { return toom3_parallel(left, right, karaCutoff, toomCutoff, parCutoff); }, iters);

    const double minTimeUs = std::min({sTimeUs, kTimeUs, tTimeUs, pTimeUs});
    const double maxSpeedup = sTimeUs / minTimeUs;

    std::cout << std::left << std::setw(12) << bits << std::setw(8) << limbs << std::setw(12)
              << ("~" + std::to_string(decDigits)) << std::setw(15) << formatDuration(sTimeUs)
              << std::setw(15) << formatDuration(kTimeUs) << std::setw(15)
              << formatDuration(tTimeUs) << std::setw(16) << formatDuration(pTimeUs);

    std::ostringstream speedupStr;
    speedupStr << std::fixed << std::setprecision(2) << maxSpeedup << "x";
    if (maxSpeedup > 1.05) {
      std::cout << color::bold(color) << color::green(color) << std::setw(16) << speedupStr.str()
                << color::reset(color);
    } else {
      std::cout << color::dim(color) << std::setw(16) << "baseline" << color::reset(color);
    }
    std::cout << '\n';
  }

  std::cout
      << '\n'
      << color::bold(color) << color::cyan(color) << "Key Algorithmic Takeaways:\n"
      << color::reset(color)
      << "  * Small sizes (<= 2,048 bits): Schoolbook O(n^2) avoids divide-and-conquer recursion.\n"
      << "  * Medium sizes (~4,096 to 8,192 bits): Karatsuba O(n^1.585) amortizes and overtakes.\n"
      << "  * Large sizes (>= 16,384 bits): Toom-3 O(n^1.465) splits into 3 sub-problems,\n"
      << "    substantially outperforming Karatsuba.\n"
      << "  * Multi-threading: Parallelizes independent recursive branches across available\n"
      << "    CPU cores for operands exceeding the parallel threshold.\n\n"
      << color::dim(color) << "Try custom operands or benchmark runs:\n"
      << "  ./build/karatsuba_cli 0b110010101 0b1011101\n"
      << "  ./build/karatsuba_cli --random 65536\n"
      << "  mise run benchmark\n"
      << "  mise run plot\n"
      << color::reset(color);
}

void runRandomTest(mp_bitcnt_t bits, std::size_t karaCutoff, std::size_t toomCutoff,
                   std::size_t parCutoff, bool verbose) {
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

  std::cout << "  Operand Width:      " << bits << " bits (" << leftLimbs << " limbs, ~"
            << decDigits << " decimal digits)\n"
            << "  Karatsuba Cutoff:   " << karaCutoff << " limbs\n"
            << "  Toom-3 Cutoff:      " << toomCutoff << " limbs\n"
            << "  Parallel Cutoff:    " << parCutoff << " limbs\n\n";

  Timer sTimer;
  sTimer.start();
  const mpz_class sProd = schoolbook(left, right);
  sTimer.stop();

  Timer kTimer;
  kTimer.start();
  const mpz_class kProd = multiply(left, right, karaCutoff);
  kTimer.stop();

  Timer tTimer;
  tTimer.start();
  const mpz_class tProd = toom3(left, right, karaCutoff, toomCutoff);
  tTimer.stop();

  Timer pTimer;
  pTimer.start();
  const mpz_class pProd = toom3_parallel(left, right, karaCutoff, toomCutoff, parCutoff);
  pTimer.stop();

  const mpz_class refProd = left * right;
  const bool verified =
      (kProd == refProd) && (sProd == refProd) && (tProd == refProd) && (pProd == refProd);

  const double sUs = sTimer.elapsedMicroseconds();
  const double kUs = kTimer.elapsedMicroseconds();
  const double tUs = tTimer.elapsedMicroseconds();
  const double pUs = pTimer.elapsedMicroseconds();

  std::cout << color::bold(color) << "Results & Comparison:" << color::reset(color) << '\n'
            << "  Status:             "
            << (verified ? std::string(color::green(color)) +
                               "[VERIFIED MATCH] (All algorithms match GMP reference)"
                         : std::string(color::red(color)) + "[ERROR] Discrepancy detected!")
            << color::reset(color) << '\n'
            << "  Schoolbook Time:    " << formatDuration(sUs) << '\n'
            << "  Karatsuba Time:     " << formatDuration(kUs) << '\n'
            << "  Toom-3 Time:        " << formatDuration(tUs) << '\n'
            << "  Parallel Time:      " << formatDuration(pUs) << '\n'
            << "  Karatsuba Speedup:  " << std::fixed << std::setprecision(2) << (sUs / kUs)
            << "x vs Schoolbook\n"
            << "  Toom-3 Speedup:     " << std::fixed << std::setprecision(2) << (sUs / tUs)
            << "x vs Schoolbook (" << (kUs / tUs) << "x vs Karatsuba)\n"
            << "  Parallel Speedup:   " << std::fixed << std::setprecision(2) << (sUs / pUs)
            << "x vs Schoolbook (" << (tUs / pUs) << "x vs Serial Toom-3)\n";

  if (verbose) {
    std::cout << "\n"
              << color::bold(color)
              << "Product Preview (first & last 30 decimal digits):" << color::reset(color) << '\n';
    std::string prodStr = refProd.get_str(10);
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
  std::size_t karaCutoff = DEFAULT_CUTOFF;
  std::size_t toomCutoff = DEFAULT_TOOM_CUTOFF;
  std::size_t parCutoff = DEFAULT_PARALLEL_CUTOFF;
  bool forceParallel = false;
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
    } else if (arg == "--parallel") {
      forceParallel = true;
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
      karaCutoff = static_cast<std::size_t>(std::stoul(argv[++i]));
    } else if ((arg == "-t" || arg == "--toom-cutoff") && i + 1 < argc) {
      toomCutoff = static_cast<std::size_t>(std::stoul(argv[++i]));
    } else if ((arg == "-p" || arg == "--parallel-cutoff") && i + 1 < argc) {
      parCutoff = static_cast<std::size_t>(std::stoul(argv[++i]));
    } else if (arg.starts_with("-")) {
      std::cerr << "Error: Unknown option '" << arg << "'. Use --help for usage.\n";
      return 1;
    } else {
      operands.push_back(arg);
    }
  }

  if (randomBits > 0) {
    runRandomTest(randomBits, karaCutoff, toomCutoff, parCutoff, verbose);
    return 0;
  }

  if (operands.empty()) {
    runShowcase(karaCutoff, toomCutoff, parCutoff);
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
  const mpz_class kProd = multiply(left, right, karaCutoff, forceParallel, parCutoff);
  kTimer.stop();

  Timer tTimer;
  tTimer.start();
  const mpz_class tProd = toom3(left, right, karaCutoff, toomCutoff, forceParallel, parCutoff);
  tTimer.stop();

  Timer sTimer;
  sTimer.start();
  const mpz_class sProd = schoolbook(left, right);
  sTimer.stop();

  const mpz_class refProd = left * right;
  const bool verified = (kProd == sProd) && (tProd == sProd) && (kProd == refProd);

  if (quiet) {
    std::cout << tProd.get_str(10) << '\n';
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
              << "  Left:            " << leftLimbs << " limbs (" << leftBits << " bits)\n"
              << "  Right:           " << rightLimbs << " limbs (" << rightBits << " bits)\n"
              << "  Karatsuba Cutoff:" << karaCutoff << " limbs\n"
              << "  Toom-3 Cutoff:   " << toomCutoff << " limbs\n\n";
  }

  std::cout << color::bold(color) << "Results:" << color::reset(color) << '\n';
  std::cout << "  Decimal:     " << color::green(color) << tProd.get_str(10) << color::reset(color)
            << '\n';
  std::cout << "  Hexadecimal: " << color::cyan(color) << "0x" << tProd.get_str(16)
            << color::reset(color) << '\n';
  if (mpz_sizeinbase(tProd.get_mpz_t(), 2) <= 128) {
    std::cout << "  Binary:      " << color::yellow(color) << "0b" << tProd.get_str(2)
              << color::reset(color) << '\n';
  } else {
    std::cout << "  Binary:      (" << mpz_sizeinbase(tProd.get_mpz_t(), 2)
              << " bits, omitted for brevity)\n";
  }

  std::cout << "\n"
            << color::bold(color) << "Verification & Timing:" << color::reset(color) << '\n';
  std::cout << "  Status:      "
            << (verified ? std::string(color::green(color)) +
                               "PASSED (All algorithms match GMP reference)"
                         : std::string(color::red(color)) + "FAILED (Discrepancy detected!)")
            << color::reset(color) << '\n';
  std::cout << "  Schoolbook:  " << formatDuration(sTimer.elapsedMicroseconds()) << '\n';
  std::cout << "  Karatsuba:   " << formatDuration(kTimer.elapsedMicroseconds()) << '\n';
  std::cout << "  Toom-3:      " << formatDuration(tTimer.elapsedMicroseconds()) << '\n';

  if (!verified) {
    return 2;
  }
  return 0;
}
