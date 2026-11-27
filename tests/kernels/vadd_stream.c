// Kernel: vadd_stream - c[i] = a[i] + b[i] on int32, no reuse (pure streaming).
// Working set 3 x 16 KiB (48 KiB), far larger than any allowed L1 D-cache.
// Shows: line size matters, cache size does not.
#include "kernel_common.h"

#define N 4096
static int32_t a[N] __attribute__((aligned(64))), b[N] __attribute__((aligned(64)));
static int32_t c_ref[N] __attribute__((aligned(64))), c_vec[N] __attribute__((aligned(64)));

static void vadd_stream_scalar(const int32_t *x, const int32_t *y, int32_t *z, int n) {
  for (int i = 0; i < n; i++) z[i] = x[i] + y[i];
}

// Implement in kernels/vec/vadd_stream_vec.c. Return 0 when implemented.
__attribute__((weak)) int vadd_stream_vector(const int32_t *x, const int32_t *y,
                                             int32_t *z, int n) {
  (void)x; (void)y; (void)z; (void)n;
  return -1;
}

int main(void) {
  fill32(a, N);
  fill32(b, N);
  perf_setup();
  MEASURE("vadd_stream", "scalar", vadd_stream_scalar(a, b, c_ref, N));
  int rc;
  MEASURE("vadd_stream", "vector", rc = vadd_stream_vector(a, b, c_vec, N));
  return kernel_result("vadd_stream", rc == 0, rc == 0 ? compare32(c_ref, c_vec, N) : 0);
}
