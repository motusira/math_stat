#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef int32_t i32;
typedef uint32_t u32;

typedef float f32;
typedef double f64;

typedef enum { NORMAL, CAUCHY, LAPLACE, POISSON, UNIFORM } GEN_TYPE;

bool verbose = false;
u32 windows_counter = 0;

f64 snd(f64 x) { return 1.0 / sqrt(2.0 * M_PI) * exp(-1.0 / 2.0 * x * x); }
f64 scd(f64 x) { return 1.0 / (M_PI * (1.0 + x * x)); }
f64 ld(f64 x) {
  return 1.0 / (2.0 * sqrt(2.0)) * exp(-1.0 / sqrt(2.0) * fabs(x));
}
f64 pd(f64 x) {
  if (x < 0.0)
    return 0.0;
  f64 lambda = 10.0;
  return exp(x * log(lambda) - lambda - lgamma(x + 1.0));
}
f64 ud(f64 x) {
  f64 limit = sqrt(3.0);
  if (x >= -limit && x <= limit)
    return 1.0 / (2.0 * limit);
  return 0.0;
}

f64 generate_num(GEN_TYPE type) {
  f64 u = (f64)rand() / RAND_MAX;
  f64 u2 = (f64)rand() / RAND_MAX;

  switch (type) {
  case NORMAL:
    return sqrt(-2.0 * log(u)) * cos(2.0 * M_PI * u2);
  case CAUCHY:
    return tan(M_PI * (u - 0.5));
  case LAPLACE: {
    f64 a = 1.0 / sqrt(2.0);
    return (u < 0.5) ? log(2.0 * u) / a : -log(2.0 * (1.0 - u)) / a;
  }
  case POISSON: {
    f64 lambda = 10.0;
    f64 L = exp(-lambda);
    f64 p = 1.0;
    i32 k = 0;
    do {
      k++;
      f64 u = (f64)rand() / RAND_MAX;
      p *= u;
    } while (p > L);
    return k - 1;
  }
  case UNIFORM: {
    f64 limit = sqrt(3.0);
    return -limit + u * 2.0 * limit;
  }
  default:
    return 0.0;
  }
}

int cmp_f64(const void *a, const void *b) {
  f64 fa = *(const f64 *)a;
  f64 fb = *(const f64 *)b;
  return (fa > fb) - (fa < fb);
}

f64 get_median(f64 *arr, i32 start, i32 end) {
  i32 len = end - start + 1;
  if (len % 2 == 0) {
    return (arr[start + len / 2 - 1] + arr[start + len / 2]) / 2.0;
  } else {
    return arr[start + len / 2];
  }
}

f64 get_outliers_proportion(GEN_TYPE type, i32 n) {
  f64 *x = (f64 *)malloc(n * sizeof(f64));
  for (i32 i = 0; i < n; i++) {
    x[i] = generate_num(type);
  }

  qsort(x, n, sizeof(f64), cmp_f64);

  i32 half_len = n / 2;
  f64 q1 = get_median(x, 0, half_len - 1);
  f64 q3 = get_median(x, n - half_len, n - 1);

  f64 iqr = q3 - q1;
  f64 lower_bound = q1 - 1.5 * iqr;
  f64 upper_bound = q3 + 1.5 * iqr;

  i32 outliers = 0;
  for (i32 i = 0; i < n; i++) {
    if (x[i] < lower_bound || x[i] > upper_bound) {
      outliers++;
    }
  }

  free(x);
  return (f64)outliers / n;
}

void plot_boxplot(FILE *pipe, GEN_TYPE type, const char *name) {
  fprintf(pipe, "set term qt %u size 600,400 title '%s Boxplot'\n",
          windows_counter++, name);
  fprintf(pipe, "set title 'Tukey Boxplot: %s'\n", name);

  fprintf(pipe, "set style data boxplot\n");
  fprintf(pipe, "set style boxplot outliers pt 7\n");
  fprintf(pipe, "set style fill solid 0.5 border -1\n");
  fprintf(pipe, "set boxwidth 0.4\n");
  fprintf(pipe, "set xtics ('n = 20' 1, 'n = 100' 2)\n");
  fprintf(pipe, "unset key\n");

  fprintf(pipe, "$data20 << EOD\n");
  for (i32 i = 0; i < 20; i++) {
    fprintf(pipe, "%lf\n", generate_num(type));
  }
  fprintf(pipe, "EOD\n");

  fprintf(pipe, "$data100 << EOD\n");
  for (i32 i = 0; i < 100; i++) {
    fprintf(pipe, "%lf\n", generate_num(type));
  }
  fprintf(pipe, "EOD\n");

  fprintf(pipe, "plot $data20 using (1):1, $data100 using (2):1\n");
  fflush(pipe);
}

void run_outlier_analysis() {
  i32 n_vals[] = {20, 100};
  GEN_TYPE types[] = {NORMAL, CAUCHY, LAPLACE, POISSON, UNIFORM};
  const char *names[] = {"Normal", "Cauchy", "Laplace", "Poisson", "Uniform"};

  printf("========================================================\n");
  printf("| Distribution     | Sample Size (n) | Outliers Prop. |\n");
  printf("========================================================\n");

  for (int t = 0; t < 5; t++) {
    for (int i = 0; i < 2; i++) {
      f64 total_prop = 0.0;
      for (int iter = 0; iter < 1000; iter++) {
        total_prop += get_outliers_proportion(types[t], n_vals[i]);
      }
      f64 average_prop = total_prop / 1000.0;
      printf("| %-16s | %-15d | %-14.4f |\n", names[t], n_vals[i],
             average_prop);
    }
    printf("--------------------------------------------------------\n");
  }
}

i32 main() {
  srand(time(NULL));

  run_outlier_analysis();

  FILE *gnuplot_pipe = popen("gnuplot -persistent", "w");
  if (!gnuplot_pipe) {
    printf("Gnuplot not found!\n");
    return 1;
  }

  plot_boxplot(gnuplot_pipe, NORMAL, "Normal Distribution");
  plot_boxplot(gnuplot_pipe, CAUCHY, "Cauchy Distribution");
  plot_boxplot(gnuplot_pipe, LAPLACE, "Laplace Distribution");
  plot_boxplot(gnuplot_pipe, POISSON, "Poisson Distribution");
  plot_boxplot(gnuplot_pipe, UNIFORM, "Uniform Distribution");

  getchar();

  pclose(gnuplot_pipe);
  return 0;
}
