#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef int32_t i32;
typedef uint32_t u32;

typedef float f32;
typedef double f64;

bool verbose = true;

u32 windows_counter = 0;

f64 snd(f64 x) { return 1.0 / sqrt(2.0 * M_PI) * exp(-1.0 / 2.0 * x * x); }

f64 scd(f64 x) { return 1.0 / (M_PI * (1.0 + x * x)); }

f64 ld(f64 x) {
  return 1.0 / (2.0 * sqrt(2.0)) * exp(-1.0 / sqrt(2.0) * fabs(x));
}

void set_styles(FILE *gnuplot_pipe) {
  fprintf(gnuplot_pipe, "set grid xtics ytics\n");
  fprintf(gnuplot_pipe,
          "set multiplot layout 2, 2 title 'Different distributions'\n");

  fflush(gnuplot_pipe);
}

void new_window(FILE *gnuplot_pipe, const char *window_name) {
  fprintf(gnuplot_pipe, "set term qt %u title '%s'\n", windows_counter,
          window_name);
  windows_counter++;
}

typedef f64 (*density_fn)(f64);

void plot_graphic(FILE *gnuplot_pipe, density_fn fn, int n,
                  const char *window_name, const char *graphic_name,
                  const char *fn_name) {
  new_window(gnuplot_pipe, window_name);

  int half_n = n / 2;
  f64 div = half_n / 5.0; // 5 by default and half in each way (neg and pos)
  f64 *x = malloc((n + 1) *
                  sizeof(f64)); // half of n in neg, half in pos and 1 for zero
  f64 *y = malloc((n + 1) * sizeof(f64));

  fprintf(gnuplot_pipe, "set title '%s with n = %d'\n", graphic_name, n);
  fprintf(gnuplot_pipe, "set xlabel 'X'\n");
  fprintf(gnuplot_pipe, "set ylabel 'Y'\n");

  fprintf(gnuplot_pipe, "set xrange [-5:5]\n");
  fprintf(gnuplot_pipe, "set xtics 1\n");

  fprintf(gnuplot_pipe, "$%s_data << EOD\n", fn_name);

  for (int i = -half_n; i <= half_n; i++) {
    x[i + half_n] = i / div;
    y[i + half_n] = fn(x[i + half_n]);
    if (verbose) {
      printf("%s(%lf) = %lf\n", fn_name, x[i + half_n], y[i + half_n]);
    }
    fprintf(gnuplot_pipe, "%lf %lf\n", x[i + half_n], y[i + half_n]);
  }

  fprintf(gnuplot_pipe, "EOD\n");

  fprintf(gnuplot_pipe, "plot $%s_data using 1:2 with linespoints title '%s'\n",
          fn_name, fn_name);

  fflush(gnuplot_pipe);

  free(x);
  free(y);

  fflush(gnuplot_pipe);
}

int main() {
  FILE *gnuplot_pipe = popen("gnuplot -persistent", "w");

  plot_graphic(gnuplot_pipe, snd, 200, "Standart normal distribution",
               "Standart normal distribution", "snd");

  plot_graphic(gnuplot_pipe, scd, 200, "Standart cauchy distribution",
               "Standart cauchy distribution", "scd");

  plot_graphic(gnuplot_pipe, ld, 200, "Standart laplace distribution",
               "Standart laplace distribution", "sld");
  pclose(gnuplot_pipe);

  return 0;
}
