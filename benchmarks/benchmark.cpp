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
#include "karatsuba/toom3.hpp"
#include "karatsuba/version.hpp"

namespace {

using Clock = std::chrono::steady_clock;

struct BenchmarkConfig {
  std::size_t karaCutoff = km::DEFAULT_CUTOFF;
  std::size_t toomCutoff = km::DEFAULT_TOOM_CUTOFF;
  std::size_t parCutoff = km::DEFAULT_PARALLEL_CUTOFF;
  std::size_t maxLimbs = 2048;
  std::size_t batches = 3;
  bool outputCsv = false;
  bool outputDat = false;
  bool outputMarkdown = false;
  bool includeGmp = true;
};

mpz_class gmpMultiply(const mpz_class& left, const mpz_class& right) {
  return left * right;
}

template <typename Fn>
double measure(Fn&& fn, const mpz_class& left, const mpz_class& right, const mpz_class& expected) {
  const auto start = Clock::now();
  std::size_t count = 0;
  mpz_class result;
  Clock::time_point end;

  do {
    result = fn(left, right);
    ++count;
    end = Clock::now();
  } while (end - start < std::chrono::milliseconds(4) && count < 1000);

  if (result != expected) {
    throw std::runtime_error("Product differs from reference value!");
  }
  return std::chrono::duration<double, std::micro>(end - start).count() /
         static_cast<double>(count);
}

void printHelp(std::string_view name) {
  std::cout << "Usage: " << name << " [OPTIONS]\n\n"
            << "Options:\n"
            << "  -h, --help               Show this help message and exit\n"
            << "  -c, --cutoff <N>         Set Karatsuba cutoff in limbs (default: 32)\n"
            << "  -t, --toom-cutoff <N>    Set Toom-3 cutoff in limbs (default: 64)\n"
            << "  -p, --parallel-cutoff <N> Set parallel cutoff in limbs (default: 256)\n"
            << "  -m, --max-limbs <N>      Set maximum operand limb size (default: 2048)\n"
            << "  -b, --batches <N>        Number of timing batches per size (default: 3)\n"
            << "  --csv                    Output results in CSV format\n"
            << "  --dat                    Output results in whitespace-separated DAT format\n"
            << "  --markdown               Output results in Markdown table format\n"
            << "  --no-gmp                 Exclude native GMP multiplication comparison\n";
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
      config.karaCutoff = static_cast<std::size_t>(std::stoul(argv[++i]));
    } else if ((arg == "-t" || arg == "--toom-cutoff") && i + 1 < argc) {
      config.toomCutoff = static_cast<std::size_t>(std::stoul(argv[++i]));
    } else if ((arg == "-p" || arg == "--parallel-cutoff") && i + 1 < argc) {
      config.parCutoff = static_cast<std::size_t>(std::stoul(argv[++i]));
    } else if ((arg == "-m" || arg == "--max-limbs") && i + 1 < argc) {
      config.maxLimbs = static_cast<std::size_t>(std::stoul(argv[++i]));
    } else if ((arg == "-b" || arg == "--batches") && i + 1 < argc) {
      config.batches = static_cast<std::size_t>(std::stoul(argv[++i]));
    } else if (arg == "--csv") {
      config.outputCsv = true;
    } else if (arg == "--dat") {
      config.outputDat = true;
    } else if (arg == "--markdown") {
      config.outputMarkdown = true;
    } else if (arg == "--no-gmp") {
      config.includeGmp = false;
    } else {
      std::cerr << "Unknown option: " << arg << "\n";
      return 1;
    }
  }

  const std::vector<std::size_t> allLimbCounts{8,   16,  24,  32,  48,   64,   96,   128,  192,
                                               256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096};

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

  const bool plainOutput = (config.outputCsv || config.outputDat || config.outputMarkdown);
  const bool color = !plainOutput && km::color::isColorSupported();

  if (config.outputCsv) {
    std::cout << "Bits,Limbs,Schoolbook_us,Karatsuba_us,Karatsuba_Par_us,Toom3_us,Toom3_Par_us";
    if (config.includeGmp) {
      std::cout << ",GMP_us";
    }
    std::cout << "\n";
  } else if (config.outputDat) {
    std::cout << "# Bits Limbs Schoolbook_us Karatsuba_us Karatsuba_Par_us Toom3_us Toom3_Par_us";
    if (config.includeGmp) {
      std::cout << " GMP_us";
    }
    std::cout << "\n";
  } else if (config.outputMarkdown) {
    std::cout << "| Bits | Limbs | Schoolbook (us) | Karatsuba (us) | Karatsuba Par (us) "
                 "| Toom-3 (us) | Toom-3 Par (us) |";
    if (config.includeGmp) {
      std::cout << " GMP Native (us) |";
    }
    std::cout << "\n| ---: | ---: | ---: | ---: | ---: | ---: | ---: |";
    if (config.includeGmp) {
      std::cout << " ---: |";
    }
    std::cout << "\n";
  } else {
    std::cout << km::color::bold(color) << km::color::cyan(color)
              << "================================================================================="
                 "=======\n"
              << "                 GMP Integer Multiplication Algorithm Benchmark\n"
              << "================================================================================="
                 "=======\n"
              << km::color::reset(color) << "Median time per product across " << config.batches
              << " batches (lower is faster).\n"
              << "Karatsuba cutoff: " << config.karaCutoff
              << " limbs, Toom-3 cutoff: " << config.toomCutoff
              << " limbs, Parallel cutoff: " << config.parCutoff << " limbs.\n\n"
              << km::color::bold(color) << std::setw(10) << "Bits" << std::setw(8) << "Limbs"
              << std::setw(15) << "Schoolbook" << std::setw(15) << "Karatsuba" << std::setw(16)
              << "Karatsuba Par" << std::setw(15) << "Toom-3" << std::setw(16) << "Toom-3 Par";
    if (config.includeGmp) {
      std::cout << std::setw(15) << "GMP Native";
    }
    std::cout << km::color::reset(color) << '\n'
              << std::string(config.includeGmp ? 110 : 95, '-') << '\n'
              << std::fixed << std::setprecision(2);
  }

  auto karaFn = [&](const mpz_class& a, const mpz_class& b) {
    return km::multiply(a, b, config.karaCutoff, false);
  };
  auto karaParFn = [&](const mpz_class& a, const mpz_class& b) {
    return km::multiply(a, b, config.karaCutoff, true, config.parCutoff);
  };
  auto toomFn = [&](const mpz_class& a, const mpz_class& b) {
    return km::toom3(a, b, config.karaCutoff, config.toomCutoff, false);
  };
  auto toomParFn = [&](const mpz_class& a, const mpz_class& b) {
    return km::toom3(a, b, config.karaCutoff, config.toomCutoff, true, config.parCutoff);
  };

  for (const std::size_t limbs : limbCounts) {
    const mp_bitcnt_t bits = static_cast<mp_bitcnt_t>(limbs) * GMP_NUMB_BITS;
    mpz_class left = random.get_z_bits(bits);
    mpz_class right = random.get_z_bits(bits);
    mpz_setbit(left.get_mpz_t(), bits - 1);
    mpz_setbit(right.get_mpz_t(), bits - 1);
    const mpz_class expected = left * right;

    std::vector<double> sTimes(config.batches);
    std::vector<double> kTimes(config.batches);
    std::vector<double> kpTimes(config.batches);
    std::vector<double> tTimes(config.batches);
    std::vector<double> tpTimes(config.batches);
    std::vector<double> gTimes(config.batches);

    for (std::size_t i = 0; i < config.batches; ++i) {
      sTimes[i] = measure(km::schoolbook, left, right, expected);
      kTimes[i] = measure(karaFn, left, right, expected);
      kpTimes[i] = measure(karaParFn, left, right, expected);
      tTimes[i] = measure(toomFn, left, right, expected);
      tpTimes[i] = measure(toomParFn, left, right, expected);
      if (config.includeGmp) {
        gTimes[i] = measure(gmpMultiply, left, right, expected);
      }
    }

    std::sort(sTimes.begin(), sTimes.end());
    std::sort(kTimes.begin(), kTimes.end());
    std::sort(kpTimes.begin(), kpTimes.end());
    std::sort(tTimes.begin(), tTimes.end());
    std::sort(tpTimes.begin(), tpTimes.end());
    std::sort(gTimes.begin(), gTimes.end());

    const std::size_t mid = config.batches / 2;
    const double sVal = sTimes[mid];
    const double kVal = kTimes[mid];
    const double kpVal = kpTimes[mid];
    const double tVal = tTimes[mid];
    const double tpVal = tpTimes[mid];
    const double gVal = config.includeGmp ? gTimes[mid] : 0.0;

    if (config.outputCsv) {
      std::cout << bits << ',' << limbs << ',' << sVal << ',' << kVal << ',' << kpVal << ',' << tVal
                << ',' << tpVal;
      if (config.includeGmp) {
        std::cout << ',' << gVal;
      }
      std::cout << '\n';
    } else if (config.outputDat) {
      std::cout << bits << ' ' << limbs << ' ' << sVal << ' ' << kVal << ' ' << kpVal << ' ' << tVal
                << ' ' << tpVal;
      if (config.includeGmp) {
        std::cout << ' ' << gVal;
      }
      std::cout << '\n';
    } else if (config.outputMarkdown) {
      std::cout << "| " << bits << " | " << limbs << " | " << std::fixed << std::setprecision(2)
                << sVal << " | " << kVal << " | " << kpVal << " | " << tVal << " | " << tpVal
                << " |";
      if (config.includeGmp) {
        std::cout << " " << gVal << " |";
      }
      std::cout << '\n';
    } else {
      std::cout << std::setw(10) << bits << std::setw(8) << limbs << std::setw(15) << sVal
                << std::setw(15) << kVal << std::setw(16) << kpVal << std::setw(15) << tVal
                << std::setw(16) << tpVal;
      if (config.includeGmp) {
        std::cout << std::setw(15) << gVal;
      }
      std::cout << '\n';
    }
  }

  return 0;
}
