// Kernel: dot - int32 dot product (low 32 bits), two read-only streams.
// The vector version needs VMUL or VMAC (stretch goals). Teams without them
// report this kernel as scalar-only.
#include "kernel_common.h"

#define N 4096
static int32_t a[N] __attribute__((aligned(64))), b[N] __attribute__((aligned(64)));

static int32_t dot_scalar(const int32_t *x, const int32_t *y, int n) {
  uint32_t acc = 0;
  for (int i = 0; i < n; i++) acc += (uint32_t)x[i] * (uint32_t)y[i];
  return (int32_t)acc;
}

// Implement in kernels/vec/dot_vec.c. Write the result to *out, return 0.
__attribute__((weak)) int dot_vector(const int32_t *x, const int32_t *y, int n, int32_t *out) {
  (void)x; (void)y; (void)n; (void)out;
  return -1;
}

int main(void) {
  fill32(a, N);
  fill32(b, N);
  perf_setup();
  int32_t r_ref, r_vec = 0;
  MEASURE("dot", "scalar", r_ref = dot_scalar(a, b, N));
  int rc;
  MEASURE("dot", "vector", rc = dot_vector(a, b, N, &r_vec));
  return kernel_result("dot", rc == 0, rc == 0 ? (r_ref != r_vec) : 0);
}
