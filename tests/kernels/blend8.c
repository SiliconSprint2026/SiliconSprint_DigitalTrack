// Kernel: blend8 - in-place 8-bit image brighten: img[i] = img[i] + delta[i]
// (modulo 256). Read-modify-write over 32 KiB (+32 KiB of deltas): shows write-back vs
// write-through traffic. Vector version needs SEW = 8 (Stage 2 vcfg).
#include "kernel_common.h"

#define N 32768
static uint8_t img_ref[N] __attribute__((aligned(64))), img_vec[N] __attribute__((aligned(64)));
static uint8_t delta[N] __attribute__((aligned(64)));

static void blend8_scalar(uint8_t *img, const uint8_t *d, int n) {
  for (int i = 0; i < n; i++) img[i] = (uint8_t)(img[i] + d[i]);
}

// Implement in kernels/vec/blend8_vec.c. Return 0 when implemented.
__attribute__((weak)) int blend8_vector(uint8_t *img, const uint8_t *d, int n) {
  (void)img; (void)d; (void)n;
  return -1;
}

int main(void) {
  fill8(img_ref, N);
  for (int i = 0; i < N; i++) img_vec[i] = img_ref[i];
  fill8(delta, N);
  perf_setup();
  MEASURE("blend8", "scalar", blend8_scalar(img_ref, delta, N));
  int rc;
  MEASURE("blend8", "vector", rc = blend8_vector(img_vec, delta, N));
  return kernel_result("blend8", rc == 0, rc == 0 ? compare8(img_ref, img_vec, N) : 0);
}
