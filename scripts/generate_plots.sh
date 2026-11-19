#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${ROOT_DIR}"

echo "[*] Running multiplication benchmarks (up to 2048 limbs)..."
./build/karatsuba_benchmark --csv --max-limbs 2048 > benchmark_results.csv
{
  echo "# Bits Limbs Schoolbook_us Karatsuba_us Karatsuba_Par_us Toom3_us Toom3_Par_us GMP_us"
  tail -n +2 benchmark_results.csv | tr ',' ' '
} > benchmark_results.dat

echo "[*] Generating benchmark plots..."
if command -v gnuplot &>/dev/null; then
  echo "[*] Using gnuplot to render curves..."
  gnuplot scripts/plot_benchmark.gp
  echo "[✓] Gnuplot successfully generated benchmark_plot.png and benchmark_plot.svg"
else
  echo "[*] Gnuplot not detected in PATH; falling back to Python / Matplotlib renderer..."
  MPLCONFIGDIR=/tmp/mpl python3 scripts/plot_benchmark.py benchmark_results.csv
fi

echo "[✓] Benchmark plotting complete."
