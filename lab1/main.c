#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef float f32;
typedef double f64;

bool verbose = true;

f64 snd(f64 x) { return 1.0 / sqrt(2.0 * M_PI) * exp(-1.0 / 2.0 * x * x); }

f64 scd(f64 x) { return 1.0 / (M_PI * (1.0 + x * x)); }

void set_styles(FILE *gnuplot_pipe) {
  fprintf(gnuplot_pipe, "set grid xtics ytics\n");
  fprintf(gnuplot_pipe,
          "set multiplot layout 2, 1 title 'Different distributions'\n");

  fflush(gnuplot_pipe);
}

void plot_snd(int n, FILE *gnuplot_pipe) {
  int half_n = n / 2;
  f64 div = half_n / 5.0; // 5 by default and half in each way (neg and pos)
  f64 *x = malloc((n + 1) *
                  sizeof(f64)); // half of n in neg, half in pos and 1 for zero
  f64 *y = malloc((n + 1) * sizeof(f64));

  fprintf(gnuplot_pipe,
          "set title 'Standart normal distribution with n = %d'\n", n);
  fprintf(gnuplot_pipe, "set xlabel 'X'\n");
  fprintf(gnuplot_pipe, "set ylabel 'Y'\n");

  fprintf(gnuplot_pipe, "set xrange [-5:5]\n");
  fprintf(gnuplot_pipe, "set xtics 1\n");

  fprintf(gnuplot_pipe, "$snd_data << EOD\n");

  for (int i = -half_n; i <= half_n; i++) {
    x[i + half_n] = i / div;
    y[i + half_n] = snd(x[i + half_n]);
    if (verbose) {
      printf("snd(%lf) = %lf\n", x[i + half_n], y[i + half_n]);
    }
    fprintf(gnuplot_pipe, "%lf %lf\n", x[i + half_n], y[i + half_n]);
  }

  fprintf(gnuplot_pipe, "EOD\n");

  fprintf(gnuplot_pipe,
          "plot $snd_data using 1:2 with linespoints title 'snd'\n");

  fflush(gnuplot_pipe);

  free(x);
  free(y);
}

void plot_scd(int n, FILE *gnuplot_pipe) {
  int half_n = n / 2;
  f64 div = half_n / 5.0; // 5 by default and half in each way (neg and pos)
  f64 *x = malloc((n + 1) *
                  sizeof(f64)); // half of n in neg, half in pos and 1 for zero
  f64 *y = malloc((n + 1) * sizeof(f64));

  fprintf(gnuplot_pipe,
          "set title 'Standart cauchy distribution with n = %d'\n", n);
  fprintf(gnuplot_pipe, "set xlabel 'X'\n");
  fprintf(gnuplot_pipe, "set ylabel 'Y'\n");

  fprintf(gnuplot_pipe, "set xrange [-5:5]\n");
  fprintf(gnuplot_pipe, "set xtics 1\n");

  fprintf(gnuplot_pipe, "$scd_data << EOD\n");

  for (int i = -half_n; i <= half_n; i++) {
    x[i + half_n] = i / div;
    y[i + half_n] = scd(x[i + half_n]);
    if (verbose) {
      printf("snd(%lf) = %lf\n", x[i + half_n], y[i + half_n]);
    }
    fprintf(gnuplot_pipe, "%lf %lf\n", x[i + half_n], y[i + half_n]);
  }

  fprintf(gnuplot_pipe, "EOD\n");

  fprintf(gnuplot_pipe,
          "plot $scd_data using 1:2 with linespoints title 'scd'\n");

  fflush(gnuplot_pipe);

  free(x);
  free(y);
}

int main() {
  FILE *gnuplot_pipe = popen("gnuplot -persistent", "w");

  set_styles(gnuplot_pipe);

  plot_snd(200, gnuplot_pipe);
  plot_scd(200, gnuplot_pipe);

  pclose(gnuplot_pipe);

  return 0;
}
