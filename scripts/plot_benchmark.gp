# Gnuplot script for arbitrary-precision multiplication benchmark curves
# Run via: gnuplot scripts/plot_benchmark.gp

set terminal pngcairo size 1280,800 enhanced font 'Sans,11' linewidth 2
set output 'benchmark_plot.png'

set title "Arbitrary-Precision Integer Multiplication Algorithm Scaling\n(Schoolbook vs Karatsuba vs Toom-Cook vs GMP Native)" font "Sans-Bold,14"
set xlabel "Operand Size (Bits)" font "Sans-Bold,11"
set ylabel "Median Execution Time (microseconds)" font "Sans-Bold,11"

set logscale x 2
set logscale y 10
set format x "2^{%L}"
set format y "10^{%T}"

set grid xtics ytics mxtics mytics lc rgb "#e0e0e0" lt 1 lw 0.8
set key top left box opaque font "Sans,10" spacing 1.3

# Color palette
# 1: Schoolbook (Red)
# 2: Karatsuba (Blue)
# 3: Karatsuba Parallel (Sky Blue dashed)
# 4: Toom-3 (Green)
# 5: Toom-3 Parallel (Teal dashed)
# 6: GMP Native (Purple)

plot 'benchmark_results.dat' using 1:3 with linespoints title "Schoolbook O(N^2)" lc rgb "#d9534f" pt 7 ps 1.2 lw 2.2, \
     'benchmark_results.dat' using 1:4 with linespoints title "Karatsuba O(N^{1.585})" lc rgb "#0275d8" pt 5 ps 1.2 lw 2.2, \
     'benchmark_results.dat' using 1:5 with linespoints title "Karatsuba Parallel" lc rgb "#5bc0de" pt 4 ps 1.2 lw 2 dt 2, \
     'benchmark_results.dat' using 1:6 with linespoints title "Toom-3 O(N^{1.465})" lc rgb "#5cb85c" pt 9 ps 1.2 lw 2.2, \
     'benchmark_results.dat' using 1:7 with linespoints title "Toom-3 Parallel" lc rgb "#20c997" pt 8 ps 1.2 lw 2 dt 2, \
     'benchmark_results.dat' using 1:8 with linespoints title "GMP Native (FFT / Asm)" lc rgb "#6f42c1" pt 11 ps 1.2 lw 2

# Also generate SVG output
set terminal svg size 1280,800 dynamic font 'Sans,11'
set output 'benchmark_plot.svg'
replot
