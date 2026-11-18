# Karatsuba multiplication benchmark

How large do two integers have to be before Karatsuba beats schoolbook
multiplication? This C++20 project runs both algorithms on the same operands
and measures the difference. It uses GNU Multiple Precision (GMP) to store
exact, arbitrary-precision integers in `mpz_class`.

## Build and run the showcase

You need CMake 3.16 or later, a C++20 compiler, and the GMP development
libraries for C and C++ (`libgmp` and `libgmpxx`). CMake uses `pkg-config`
when available.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DKARATSUBA_BUILD_CLI=OFF -DBUILD_TESTING=OFF
cmake --build build --parallel
./build/karatsuba_benchmark
```

This configuration builds the benchmark as the only executable. Use a
Release build to measure the optimized implementations.

## Example results

These rows are from a Release run on an Intel Core Ultra 5 125H with GCC
16.2.1 and GMP 6.3.0. Times are microseconds per multiplication.

| Bits per operand | Schoolbook (µs) | Karatsuba (µs) | Karatsuba speedup |
| ---: | ---: | ---: | ---: |
| 2,560 | 0.757 | 0.883 | 0.86× |
| 4,096 | 1.939 | 1.861 | 1.04× |
| 16,384 | 34.901 | 19.677 | 1.77× |
| 65,536 | 560.806 | 196.978 | 2.85× |
| 131,072 | 2,205.350 | 620.818 | 3.55× |

The speedup is schoolbook time divided by Karatsuba time, so values above
`1×` favor Karatsuba. The first sampled win in this run was at **4,096 bits
per operand**. Nearby sizes can switch order between runs. For a live
demonstration, compare the small rows with the largest rows and quote the
numbers from that run. The default output also shows native GMP multiplication
as a third reference.

## Why the gap grows

Schoolbook calculates the partial products between every pair of limbs. A
limb is a machine-word-sized block of an integer. For equal-width operands
with `n` limbs, schoolbook takes $O(n^2)$ work.

Karatsuba splits each operand into high and low halves. It computes the two
matching-half products and one product of the sums of the halves. Subtracting
the matching-half products from the third product gives the cross term.
Reducing four half-size multiplications to three brings the running time to
$O(n^{\log_2 3})$, about $O(n^{1.585})$. Splitting and recombining still cost
time, so the implementation uses schoolbook when the smaller operand has
32 limbs or fewer by default. The table marks these rows as `base case`.

## Benchmark settings

Each size uses a reproducible pair of random operands with the same bit
width. By default, the benchmark reports the median of five timed batches
per algorithm and size. It checks the product at the end of each batch
against native GMP multiplication outside the timed interval.

```sh
./build/karatsuba_benchmark --help
./build/karatsuba_benchmark --no-gmp
./build/karatsuba_benchmark --csv > results.csv
./build/karatsuba_benchmark --markdown > results.md
```

Use `--max-limbs` to change the largest operand size, `--cutoff` to change
when Karatsuba switches to schoolbook, and `--batches` to change the number
of timing batches. With 64-bit GMP limbs, the default maximum of 2,048 limbs
is 131,072 bits per operand.

## Run the tests

```sh
cmake -S . -B build-test -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build-test --parallel
(cd build-test && ctest --output-on-failure)
```

The tests cover signs, zero, unequal operand widths, and randomized products.
If you use `mise`, the repository also defines `mise run benchmark` and
`mise run test`.
