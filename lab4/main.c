#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef int32_t i32;
typedef uint32_t u32;
typedef double f64;

typedef enum { NORMAL, CAUCHY, LAPLACE, POISSON, UNIFORM } GEN_TYPE;

f64 snd(f64 x) { return 1.0 / sqrt(2.0 * M_PI) * exp(-0.5 * x * x); }
f64 scd(f64 x) { return 1.0 / (M_PI * (1.0 + x * x)); }
f64 ld(f64 x) {
  return 1.0 / (2.0 * sqrt(2.0)) * exp(-1.0 / sqrt(2.0) * fabs(x));
}
f64 pd(f64 x) {
  if (x < 0.0)
    return 0.0;
  f64 lambda = 10.0;
  return pow(lambda, x) * exp(-lambda) / tgamma(x + 1.0);
}
f64 ud(f64 x) {
  f64 limit = sqrt(3.0);
  return (x >= -limit && x <= limit) ? 1.0 / (2.0 * limit) : 0.0;
}

f64 sn_cdf(f64 x) { return 0.5 * (1.0 + erf(x / sqrt(2.0))); }
f64 sc_cdf(f64 x) { return atan(x) / M_PI + 0.5; }
f64 l_cdf(f64 x) {
  f64 a = 1.0 / sqrt(2.0);
  return (x < 0) ? 0.5 * exp(a * x) : 1.0 - 0.5 * exp(-a * x);
}
f64 p_cdf(f64 x) {
  if (x < 0)
    return 0.0;
  f64 sum = 0, lambda = 10.0;
  for (int k = 0; k <= (int)floor(x); k++) {
    f64 p = exp(-lambda);
    for (int j = 1; j <= k; j++)
      p *= (lambda / j);
    sum += p;
  }
  return sum;
}
f64 u_cdf(f64 x) {
  f64 limit = sqrt(3.0);
  if (x < -limit)
    return 0.0;
  if (x > limit)
    return 1.0;
  return (x + limit) / (2.0 * limit);
}

f64 generate_num(GEN_TYPE type) {
  f64 u = (f64)rand() / RAND_MAX;
  f64 u2 = (f64)rand() / RAND_MAX;
  switch (type) {
  case NORMAL:
    return sqrt(-2.0 * log(u)) * cos(2.0 * M_PI * u2);
  case CAUCHY:
    return tan(M_PI * (u - 0.5));
  case LAPLACE:
    return (u < 0.5) ? log(2.0 * u) * sqrt(0.5)
                     : -log(2.0 * (1.0 - u)) * sqrt(0.5);
  case POISSON: {
    f64 L = exp(-10.0), p = 1.0;
    i32 k = 0;
    do {
      k++;
      p *= (f64)rand() / RAND_MAX;
    } while (p > L);
    return k - 1;
  }
  case UNIFORM:
    return -sqrt(3.0) + ((f64)rand() / RAND_MAX) * 2.0 * sqrt(3.0);
  default:
    return 0;
  }
}

int cmp(const void *a, const void *b) {
  f64 fa = *(const f64 *)a, fb = *(const f64 *)b;
  return (fa > fb) - (fa < fb);
}

void plot_lab4(GEN_TYPE type, i32 n, const char *name, const char *filename,
               f64 (*pdf)(f64), f64 (*cdf)(f64)) {

  FILE *gp = popen("gnuplot", "w");
  if (!gp) {
    perror("Failed to open Gnuplot");
    return;
  }

  f64 min = (type == POISSON) ? 6.0 : -4.0;
  f64 max = (type == POISSON) ? 14.0 : 4.0;

  f64 *sample = malloc(n * sizeof(f64));
  f64 sum = 0, sum_sq = 0;
  for (int i = 0; i < n; i++) {
    sample[i] = generate_num(type);
    sum += sample[i];
  }
  qsort(sample, n, sizeof(f64), cmp);

  f64 mean = sum / n;
  for (int i = 0; i < n; i++)
    sum_sq += pow(sample[i] - mean, 2);
  f64 s = sqrt(sum_sq / (n - 1));
  f64 h = 1.06 * s * pow(n, -0.2);
  if (h < 1e-5)
    h = 0.5;

  f64 mse_cdf = 0.0;
  f64 mse_pdf = 0.0;
  i32 grid_points = 0;
  f64 step = 0.05;

  for (f64 x = min; x <= max; x += step) {
    i32 count = 0;
    while (count < n && sample[count] <= x) {
      count++;
    }
    f64 ecdf_val = (f64)count / n;

    f64 kde_val = 0.0;
    for (int i = 0; i < n; i++) {
      kde_val += exp(-0.5 * pow((x - sample[i]) / h, 2)) / sqrt(2 * M_PI);
    }
    kde_val /= (n * h);

    f64 theor_cdf = cdf(x);
    f64 theor_pdf = pdf(x);

    mse_cdf += pow(ecdf_val - theor_cdf, 2);
    mse_pdf += pow(kde_val - theor_pdf, 2);
    grid_points++;
  }

  mse_cdf /= grid_points;
  mse_pdf /= grid_points;

  printf("%-10s | %4d | %.6lf  | %.6lf\n", name, n, mse_cdf, mse_pdf);

  fprintf(gp, "set terminal pngcairo size 800,600\n");
  fprintf(gp, "set output '%s.png'\n", filename);

  fprintf(gp, "$ecdf_data << EOD\n");
  for (int i = 0; i < n; i++)
    fprintf(gp, "%lf %lf\n", sample[i], (f64)i / n);
  fprintf(gp, "EOD\n");

  fprintf(gp, "$cdf_data << EOD\n");
  for (f64 x = min; x <= max; x += 0.1)
    fprintf(gp, "%lf %lf\n", x, cdf(x));
  fprintf(gp, "EOD\n");

  fprintf(gp, "$kde_data << EOD\n");
  for (f64 x = min; x <= max; x += step) {
    f64 kde = 0;
    for (int i = 0; i < n; i++)
      kde += exp(-0.5 * pow((x - sample[i]) / h, 2)) / sqrt(2 * M_PI);
    fprintf(gp, "%lf %lf\n", x, kde / (n * h));
  }
  fprintf(gp, "EOD\n");

  fprintf(gp, "$pdf_data << EOD\n");
  for (f64 x = min; x <= max; x += step)
    fprintf(gp, "%lf %lf\n", x, pdf(x));
  fprintf(gp, "EOD\n");

  fprintf(gp, "set multiplot layout 1,2 title '%s (n=%d)'\n", name, n);
  fprintf(gp, "set grid\n");

  fprintf(gp, "set title 'ECDF vs Theoretical CDF'\n");
  fprintf(gp, "plot $ecdf_data with steps lw 2 title 'Empirical', $cdf_data "
              "with lines lc 'red' title 'Theoretical'\n");

  fprintf(gp, "set title 'KDE vs Theoretical PDF'\n");
  fprintf(gp, "plot $kde_data with lines lw 2 title 'KDE', $pdf_data with "
              "lines lc 'red' title 'Theoretical'\n");

  fprintf(gp, "unset multiplot\n");
  pclose(gp);
  free(sample);
}

int main() {
  srand(time(NULL));
  i32 sizes[] = {20, 60, 100};
  char *names[] = {"Normal", "Cauchy", "Laplace", "Poisson", "Uniform"};
  char *prefix[] = {"n", "c", "l", "p", "u"};
  GEN_TYPE types[] = {NORMAL, CAUCHY, LAPLACE, POISSON, UNIFORM};
  f64 (*pdfs[])(f64) = {snd, scd, ld, pd, ud};
  f64 (*cdfs[])(f64) = {sn_cdf, sc_cdf, l_cdf, p_cdf, u_cdf};

  printf("=========================================\n");
  printf("Distrib    |  n   | MSE (CDF) | MSE (PDF)\n");
  printf("=========================================\n");

  for (int t = 0; t < 5; t++) {
    for (int s = 0; s < 3; s++) {
      char fname[20];
      sprintf(fname, "%s_%d", prefix[t], sizes[s]);
      plot_lab4(types[t], sizes[s], names[t], fname, pdfs[t], cdfs[t]);
    }
    printf("-----------------------------------------\n");
  }
  printf("\nAll plots saved as PNG files successfully.\n");
  return 0;
}
