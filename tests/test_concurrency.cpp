#include <gmpxx.h>

#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <vector>

#include "karatsuba/karatsuba.hpp"
#include "karatsuba/thread_pool.hpp"
#include "karatsuba/toom3.hpp"
#include "test_framework.hpp"

using namespace km;

TEST_CASE(Concurrency, ThreadPoolDirectStress) {
  ThreadPool pool(4);
  constexpr std::size_t numTasks = 200;
  std::atomic<std::size_t> completed{0};
  std::vector<std::future<int>> futures;
  futures.reserve(numTasks);

  for (std::size_t i = 0; i < numTasks; ++i) {
    futures.push_back(pool.enqueue([&completed, i]() -> int {
      completed.fetch_add(1, std::memory_order_relaxed);
      return static_cast<int>(i * 2);
    }));
  }

  for (std::size_t i = 0; i < numTasks; ++i) {
    const int val = futures[i].get();
    ASSERT_EQ(val, static_cast<int>(i * 2));
  }
  ASSERT_EQ(completed.load(), numTasks);
}

TEST_CASE(Concurrency, ConcurrentMultiplicationAcrossThreads) {
  constexpr std::size_t numThreads = 6;
  constexpr std::size_t opsPerThread = 15;
  std::vector<std::thread> workers;
  std::atomic<bool> failure{false};

  for (std::size_t t = 0; t < numThreads; ++t) {
    workers.emplace_back([t, &failure]() {
      try {
        gmp_randclass rng(gmp_randinit_mt);
        rng.seed(1000UL + t * 37);

        for (std::size_t i = 0; i < opsPerThread; ++i) {
          const mp_bitcnt_t bits = 4096 + (i * 512);
          mpz_class a = rng.get_z_bits(bits);
          mpz_class b = rng.get_z_bits(bits);
          mpz_setbit(a.get_mpz_t(), bits - 1);
          mpz_setbit(b.get_mpz_t(), bits - 1);

          const mpz_class expected = a * b;

          const mpz_class kProd = multiply(a, b);
          if (kProd != expected) {
            failure.store(true);
            return;
          }

          const mpz_class tProd = toom3(a, b);
          if (tProd != expected) {
            failure.store(true);
            return;
          }
        }
      } catch (...) {
        failure.store(true);
      }
    });
  }

  for (auto& w : workers) {
    if (w.joinable()) {
      w.join();
    }
  }

  ASSERT_FALSE(failure.load());
}

TEST_CASE(Concurrency, ParallelKaratsubaAndToom3Stress) {
  constexpr std::size_t numThreads = 4;
  std::vector<std::thread> workers;
  std::atomic<bool> failure{false};

  for (std::size_t t = 0; t < numThreads; ++t) {
    workers.emplace_back([t, &failure]() {
      try {
        gmp_randclass rng(gmp_randinit_mt);
        rng.seed(5000UL + t * 53);

        for (std::size_t limbs : {128, 256, 384}) {
          const mp_bitcnt_t bits = static_cast<mp_bitcnt_t>(limbs) * GMP_NUMB_BITS;
          mpz_class a = rng.get_z_bits(bits);
          mpz_class b = rng.get_z_bits(bits);
          mpz_setbit(a.get_mpz_t(), bits - 1);
          mpz_setbit(b.get_mpz_t(), bits - 1);

          const mpz_class expected = a * b;

          // Multi-threaded Karatsuba
          const mpz_class kPar = multiply_parallel(a, b, DEFAULT_CUTOFF, 64);
          if (kPar != expected) {
            failure.store(true);
            return;
          }

          // Multi-threaded Toom-3
          const mpz_class tPar = toom3_parallel(a, b, DEFAULT_CUTOFF, DEFAULT_TOOM_CUTOFF, 64);
          if (tPar != expected) {
            failure.store(true);
            return;
          }
        }
      } catch (...) {
        failure.store(true);
      }
    });
  }

  for (auto& w : workers) {
    if (w.joinable()) {
      w.join();
    }
  }

  ASSERT_FALSE(failure.load());
}

TEST_CASE(Concurrency, HighThreadContentionMultiplication) {
  constexpr std::size_t numThreads = 12;
  constexpr std::size_t opsPerThread = 10;
  std::vector<std::thread> workers;
  std::atomic<bool> failure{false};

  for (std::size_t t = 0; t < numThreads; ++t) {
    workers.emplace_back([t, &failure]() {
      try {
        gmp_randclass rng(gmp_randinit_mt);
        rng.seed(7000UL + t * 41);

        for (std::size_t i = 0; i < opsPerThread; ++i) {
          const mp_bitcnt_t bits = 2048 + (i * 256);
          mpz_class a = rng.get_z_bits(bits);
          mpz_class b = rng.get_z_bits(bits);
          mpz_setbit(a.get_mpz_t(), bits - 1);
          mpz_setbit(b.get_mpz_t(), bits - 1);

          const mpz_class expected = a * b;
          const mpz_class kRes = multiply_parallel(a, b, DEFAULT_CUTOFF, 64);
          if (kRes != expected) {
            failure.store(true);
            return;
          }

          const mpz_class tRes = toom3_parallel(a, b, DEFAULT_CUTOFF, DEFAULT_TOOM_CUTOFF, 64);
          if (tRes != expected) {
            failure.store(true);
            return;
          }
        }
      } catch (...) {
        failure.store(true);
      }
    });
  }

  for (auto& w : workers) {
    if (w.joinable()) {
      w.join();
    }
  }

  ASSERT_FALSE(failure.load());
}
