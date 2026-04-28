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

bool verbose = true;

u32 windows_counter = 0;

f64 snd(f64 x) { return 1.0 / sqrt(2.0 * M_PI) * exp(-1.0 / 2.0 * x * x); }

f64 scd(f64 x) { return 1.0 / (M_PI * (1.0 + x * x)); }

f64 ld(f64 x) {
  return 1.0 / (2.0 * sqrt(2.0)) * exp(-1.0 / sqrt(2.0) * fabs(x));
}

f64 pd(f64 x) {
  if (x < 0.0) {
    return 0.0;
  }
  f64 lambda = 10.0;
  return pow(lambda, x) * exp(-lambda) / tgamma(x + 1.0);
}

f64 ud(f64 x) {
  f64 limit = sqrt(3.0);

  if (x >= -limit && x <= limit) {
    return 1.0 / (2.0 * limit);
  }
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
    f64 u = (f64)rand() / RAND_MAX;
    f64 limit = sqrt(3.0);
    return -limit + u * 2.0 * limit;
  }
  }
}

int compare_f64(const void *a, const void *b) {
  f64 fa = *(const f64 *)a;
  f64 fb = *(const f64 *)b;
  return (fa > fb) - (fa < fb);
}

void run_lab2_stats(GEN_TYPE type, const char *dist_name, i32 n,
                    i32 repetitions) {
  f64 sum_mean = 0, sq_sum_mean = 0;
  f64 sum_med = 0, sq_sum_med = 0;
  f64 sum_zr = 0, sq_sum_zr = 0;
  f64 sum_zq = 0, sq_sum_zq = 0;
  f64 sum_ztr = 0, sq_sum_ztr = 0;

  f64 *arr = malloc(n * sizeof(f64));

  for (i32 rep = 0; rep < repetitions; rep++) {
    for (i32 i = 0; i < n; i++) {
      arr[i] = generate_num(type);
    }

    qsort(arr, n, sizeof(f64), compare_f64);

    f64 mean = 0;
    for (i32 i = 0; i < n; i++)
      mean += arr[i];
    mean /= n;

    f64 med = (n % 2 == 0) ? (arr[n / 2 - 1] + arr[n / 2]) / 2.0 : arr[n / 2];

    f64 zr = (arr[0] + arr[n - 1]) / 2.0;

    f64 zq = (arr[n / 4] + arr[(3 * n) / 4]) / 2.0;

    i32 r = n / 10;
    f64 ztr = 0;
    for (i32 i = r; i < n - r; i++) {
      ztr += arr[i];
    }
    ztr /= (n - 2 * r);

    sum_mean += mean;
    sq_sum_mean += mean * mean;
    sum_med += med;
    sq_sum_med += med * med;
    sum_zr += zr;
    sq_sum_zr += zr * zr;
    sum_zq += zq;
    sq_sum_zq += zq * zq;
    sum_ztr += ztr;
    sq_sum_ztr += ztr * ztr;
  }

  free(arr);

  f64 E_mean = sum_mean / repetitions;
  f64 D_mean = sq_sum_mean / repetitions - E_mean * E_mean;

  f64 E_med = sum_med / repetitions;
  f64 D_med = sq_sum_med / repetitions - E_med * E_med;

  f64 E_zr = sum_zr / repetitions;
  f64 D_zr = sq_sum_zr / repetitions - E_zr * E_zr;

  f64 E_zq = sum_zq / repetitions;
  f64 D_zq = sq_sum_zq / repetitions - E_zq * E_zq;

  f64 E_ztr = sum_ztr / repetitions;
  f64 D_ztr = sq_sum_ztr / repetitions - E_ztr * E_ztr;

  printf("Distribution: %s, n = %d\n", dist_name, n);
  printf("------------------------------------------\n");
  printf("Metric    | E(z)       | D(z)\n");
  printf("------------------------------------------\n");
  printf("Mean      | %10.6f | %10.6f\n", E_mean, fabs(D_mean));
  printf("Median    | %10.6f | %10.6f\n", E_med, fabs(D_med));
  printf("z_R       | %10.6f | %10.6f\n", E_zr, fabs(D_zr));
  printf("z_Q       | %10.6f | %10.6f\n", E_zq, fabs(D_zq));
  printf("z_tr      | %10.6f | %10.6f\n", E_ztr, fabs(D_ztr));
  printf("==========================================\n\n");
}

i32 main() {
  srand(time(NULL));

  i32 data_size[3] = {10, 100, 1000};
  i32 repetitions = 1000;

  const char *dist_names[] = {"NORMAL", "CAUCHY", "LAPLACE", "POISSON",
                              "UNIFORM"};
  GEN_TYPE dist_types[] = {NORMAL, CAUCHY, LAPLACE, POISSON, UNIFORM};

  for (int d = 0; d < 5; d++) {
    for (int i = 0; i < 3; i++) {
      run_lab2_stats(dist_types[d], dist_names[d], data_size[i], repetitions);
    }
  }
  return 0;
}
