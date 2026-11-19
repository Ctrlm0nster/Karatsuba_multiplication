#!/usr/bin/env python3
"""
Benchmark Plot Generator
Reads benchmark CSV/DAT output and produces benchmark_plot.png and benchmark_plot.svg.
"""

import sys
import os
import csv

# Set writable cache dir for matplotlib
os.environ.setdefault("MPLCONFIGDIR", "/tmp/mpl")
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

def main():
    csv_file = sys.argv[1] if len(sys.argv) > 1 else "benchmark_results.csv"
    if not os.path.isfile(csv_file):
        print(f"Error: {csv_file} does not exist.")
        sys.exit(1)

    bits = []
    limbs = []
    schoolbook = []
    karatsuba = []
    karatsuba_par = []
    toom3 = []
    toom3_par = []
    gmp = []

    with open(csv_file, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            bits.append(int(row["Bits"]))
            limbs.append(int(row["Limbs"]))
            schoolbook.append(float(row["Schoolbook_us"]))
            karatsuba.append(float(row["Karatsuba_us"]))
            karatsuba_par.append(float(row["Karatsuba_Par_us"]))
            toom3.append(float(row["Toom3_us"]))
            toom3_par.append(float(row["Toom3_Par_us"]))
            if "GMP_us" in row and row["GMP_us"]:
                gmp.append(float(row["GMP_us"]))

    plt.figure(figsize=(11, 7), dpi=150)
    plt.style.use("seaborn-v0_8-whitegrid" if "seaborn-v0_8-whitegrid" in plt.style.available else "default")

    plt.plot(bits, schoolbook, label=r"Schoolbook $\mathcal{O}(N^2)$", color="#d9534f", marker="o", linewidth=2.2, markersize=6)
    plt.plot(bits, karatsuba, label=r"Karatsuba $\mathcal{O}(N^{1.585})$", color="#0275d8", marker="s", linewidth=2.2, markersize=6)
    plt.plot(bits, karatsuba_par, label="Karatsuba Parallel (Threaded)", color="#5bc0de", marker="^", linewidth=1.8, linestyle="--", markersize=6)
    plt.plot(bits, toom3, label=r"Toom-3 $\mathcal{O}(N^{1.465})$", color="#5cb85c", marker="D", linewidth=2.2, markersize=6)
    plt.plot(bits, toom3_par, label="Toom-3 Parallel (Threaded)", color="#20c997", marker="v", linewidth=1.8, linestyle="--", markersize=6)
    if gmp:
        plt.plot(bits, gmp, label="GMP Native (mpz_mul / FFT)", color="#6f42c1", marker="P", linewidth=2.0, linestyle=":", markersize=6)

    plt.xscale("log", base=2)
    plt.yscale("log", base=10)

    plt.title("Arbitrary-Precision Integer Multiplication Algorithm Scaling\nSchoolbook vs Karatsuba vs Toom-Cook vs GMP", fontsize=13, fontweight="bold", pad=12)
    plt.xlabel("Operand Width (Bits)", fontsize=11, fontweight="bold")
    plt.ylabel(r"Execution Time ($\mu$s, lower is faster)", fontsize=11, fontweight="bold")
    plt.grid(True, which="both", ls="--", alpha=0.5)
    plt.legend(loc="upper left", frameon=True, fontsize=10, shadow=True)

    plt.tight_layout()
    png_path = "benchmark_plot.png"
    svg_path = "benchmark_plot.svg"
    plt.savefig(png_path)
    plt.savefig(svg_path)
    print(f"[✓] Benchmark plots successfully generated:\n  - {png_path}\n  - {svg_path}")

if __name__ == "__main__":
    main()
