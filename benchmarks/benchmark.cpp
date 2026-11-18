#include <gmpxx.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "karatsuba/karatsuba.hpp"
#include "karatsuba/schoolbook.hpp"
#include "karatsuba/terminal_color.hpp"
#include "karatsuba/version.hpp"

namespace {

using Clock = std::chrono::steady_clock;
using MultiplyFn = mpz_class (*)(const mpz_class&, const mpz_class&);

struct BenchmarkConfig {
  std::size_t cutoff = km::DEFAULT_CUTOFF;
  std::size_t maxLimbs = 2048;
  std::size_t batches = 5;
  bool outputCsv = false;
  bool outputMarkdown = false;
  bool includeGmp = true;
};

mpz_class gmpMultiply(const mpz_class& left, const mpz_class& right) {
  return left * right;
}

double measure(MultiplyFn fn, const mpz_class& left, const mpz_class& right,
               const mpz_class& expected) {
  const auto start = Clock::now();
  std::size_t count = 0;
  mpz_class result;
  Clock::time_point end;

  do {
    for (int i = 0; i < 16; ++i) {
      result = fn(left, right);
    }
    count += 16;
    end = Clock::now();
  } while (end - start < std::chrono::milliseconds(5));

  if (result != expected) {
    throw std::runtime_error("Product differs from reference value!");
  }
  return std::chrono::duration<double, std::micro>(end - start).count() /
         static_cast<double>(count);
}

void printHelp(std::string_view name) {
  std::cout << "Usage: " << name << " [OPTIONS]\n\n"
            << "Options:\n"
            << "  -h, --help            Show this help message and exit\n"
            << "  -c, --cutoff <N>      Set Karatsuba cutoff in limbs (default: 32)\n"
            << "  -m, --max-limbs <N>   Set maximum operand limb size (default: "
               "2048)\n"
            << "  -b, --batches <N>     Number of timing batches per size (default: "
               "5)\n"
            << "  --csv                 Output results in CSV format\n"
            << "  --markdown            Output results in Markdown table format\n"
            << "  --no-gmp              Exclude native GMP multiplication "
               "comparison\n";
}

}  // namespace

int main(int argc, char* argv[]) {
  BenchmarkConfig config;

  for (int i = 1; i < argc; ++i) {
    const std::string_view arg(argv[i]);
    if (arg == "-h" || arg == "--help") {
      printHelp(argv[0]);
      return 0;
    }
    if ((arg == "-c" || arg == "--cutoff") && i + 1 < argc) {
      config.cutoff = static_cast<std::size_t>(std::stoul(argv[++i]));
    } else if ((arg == "-m" || arg == "--max-limbs") && i + 1 < argc) {
      config.maxLimbs = static_cast<std::size_t>(std::stoul(argv[++i]));
    } else if ((arg == "-b" || arg == "--batches") && i + 1 < argc) {
      config.batches = static_cast<std::size_t>(std::stoul(argv[++i]));
    } else if (arg == "--csv") {
      config.outputCsv = true;
    } else if (arg == "--markdown") {
      config.outputMarkdown = true;
    } else if (arg == "--no-gmp") {
      config.includeGmp = false;
    } else {
      std::cerr << "Unknown option: " << arg << "\n";
      return 1;
    }
  }

  const std::vector<std::size_t> allLimbCounts{8,   16,  24,   32,   40,   48,   64,  80,
                                               96,  128, 160,  192,  256,  320,  384, 512,
                                               640, 768, 1024, 1536, 2048, 3072, 4096};

  std::vector<std::size_t> limbCounts;
  for (std::size_t count : allLimbCounts) {
    if (count <= config.maxLimbs) {
      limbCounts.push_back(count);
    }
  }
  if (limbCounts.empty() || limbCounts.back() < config.maxLimbs) {
    limbCounts.push_back(config.maxLimbs);
  }

  gmp_randclass random(gmp_randinit_mt);
  random.seed(42UL);

  const bool color = (!config.outputCsv && !config.outputMarkdown) && km::color::isColorSupported();
  mp_bitcnt_t firstWin = 0;

  if (config.outputCsv) {
    std::cout << "Bits,Limbs,Schoolbook_us,Karatsuba_us,Speedup";
    if (config.includeGmp) {
      std::cout << ",GMP_us";
    }
    std::cout << "\n";
  } else if (config.outputMarkdown) {
    std::cout << "| Bits | Limbs | Schoolbook (us) | Karatsuba (us) | "
                 "Karatsuba Speedup |";
    if (config.includeGmp) {
      std::cout << " GMP Native (us) |";
    }
    std::cout << "\n| ---: | ---: | ---: | ---: | ---: |";
    if (config.includeGmp) {
      std::cout << " ---: |";
    }
    std::cout << "\n";
  } else {
    std::cout << km::color::bold(color) << km::color::cyan(color)
              << "============================================================="
                 "===========\n"
              << "                 GMP Integer Multiplication Benchmark\n"
              << "============================================================="
                 "===========\n"
              << km::color::reset(color) << "Median time per product across " << config.batches
              << " batches. Lower is faster.\n"
              << "Karatsuba recursion cutoff: " << config.cutoff << " limbs ("
              << (config.cutoff * GMP_NUMB_BITS) << " bits).\n\n"
              << km::color::bold(color) << std::setw(14) << "Bits" << std::setw(12) << "Limbs"
              << std::setw(18) << "Schoolbook (us)" << std::setw(18) << "Karatsuba (us)"
              << std::setw(20) << "Karatsuba Speedup";
    if (config.includeGmp) {
      std::cout << std::setw(18) << "GMP Native (us)";
    }
    std::cout << km::color::reset(color) << '\n'
              << std::string(config.includeGmp ? 100 : 82, '-') << '\n'
              << std::fixed << std::setprecision(3);
  }

  // Wrapper for karatsuba with cutoff
  auto karatsubaWithCutoff = [](const mpz_class& a, const mpz_class& b) -> mpz_class {
    return km::multiply(a, b);
  };

  for (const std::size_t limbs : limbCounts) {
    const mp_bitcnt_t bits = static_cast<mp_bitcnt_t>(limbs) * GMP_NUMB_BITS;
    mpz_class left = random.get_z_bits(bits);
    mpz_class right = random.get_z_bits(bits);
    mpz_setbit(left.get_mpz_t(), bits - 1);
    mpz_setbit(right.get_mpz_t(), bits - 1);
    const mpz_class expected = left * right;

    std::vector<double> schoolbookTimes(config.batches);
    std::vector<double> karatsubaTimes(config.batches);
    std::vector<double> gmpTimes(config.batches);

    for (std::size_t i = 0; i < config.batches; ++i) {
      if (i % 2 == 0) {
        schoolbookTimes[i] = measure(km::schoolbook, left, right, expected);
        karatsubaTimes[i] = measure(karatsubaWithCutoff, left, right, expected);
      } else {
        karatsubaTimes[i] = measure(karatsubaWithCutoff, left, right, expected);
        schoolbookTimes[i] = measure(km::schoolbook, left, right, expected);
      }
      if (config.includeGmp) {
        gmpTimes[i] = measure(gmpMultiply, left, right, expected);
      }
    }

    std::sort(schoolbookTimes.begin(), schoolbookTimes.end());
    std::sort(karatsubaTimes.begin(), karatsubaTimes.end());
    std::sort(gmpTimes.begin(), gmpTimes.end());

    const double sTime = schoolbookTimes[config.batches / 2];
    const double kTime = karatsubaTimes[config.batches / 2];
    const double gTime = config.includeGmp ? gmpTimes[config.batches / 2] : 0.0;

    const bool recursed = (limbs > config.cutoff);
    const bool kWon = (kTime < sTime);
    const double speedup = sTime / kTime;

    if (recursed && kWon && firstWin == 0) {
      firstWin = bits;
    }

    if (config.outputCsv) {
      std::cout << bits << ',' << limbs << ',' << sTime << ',' << kTime << ',' << speedup;
      if (config.includeGmp) {
        std::cout << ',' << gTime;
      }
      std::cout << '\n';
    } else if (config.outputMarkdown) {
      std::cout << "| " << bits << " | " << limbs << " | " << std::fixed << std::setprecision(3)
                << sTime << " | " << kTime << " | ";
      if (recursed) {
        std::cout << speedup << "x";
      } else {
        std::cout << "base case";
      }
      std::cout << " |";
      if (config.includeGmp) {
        std::cout << " " << gTime << " |";
      }
      std::cout << '\n';
    } else {
      std::cout << std::setw(14) << bits << std::setw(12) << limbs << std::setw(18) << sTime
                << std::setw(18) << kTime;

      if (recursed) {
        std::cout << (kWon ? km::color::green(color) : km::color::yellow(color)) << std::setw(19)
                  << speedup << 'x' << km::color::reset(color);
      } else {
        std::cout << std::setw(20) << "base case";
      }

      if (config.includeGmp) {
        std::cout << std::setw(18) << gTime;
      }
      std::cout << '\n';
    }
  }

  if (!config.outputCsv && !config.outputMarkdown) {
    std::cout << '\n';
    if (firstWin == 0) {
      std::cout << km::color::yellow(color)
                << "Karatsuba did not outperform schoolbook at the sampled sizes.\n"
                << km::color::reset(color);
    } else {
      std::cout << km::color::bold(color) << km::color::green(color)
                << "[*] First sampled Karatsuba win: " << firstWin << " bits per operand ("
                << (firstWin / GMP_NUMB_BITS) << " limbs).\n"
                << km::color::reset(color);
    }
  }

  return 0;
}
