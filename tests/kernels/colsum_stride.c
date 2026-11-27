// Kernel: colsum_stride - sums of the first NCOL columns of a 32 x 1024 int32
// matrix, walked column-first. Consecutive accesses are 4 KiB apart (power-of-two stride),
// so they map to the same cache set: shows conflict misses and the value of
// associativity.
#include "kernel_common.h"

#define ROWS 32
#define COLS 1024   // row pitch 4 KiB
#define NCOL 256    // columns summed (footprint 32 x 256 x 4 B = 32 KiB)
static int32_t m[ROWS][COLS] __attribute__((aligned(64)));
static int32_t s_ref[NCOL], s_vec[NCOL];

static void colsum_scalar(int32_t mat[ROWS][COLS], int32_t *s) {
  for (int c = 0; c < NCOL; c++) {
    int32_t acc = 0;
    for (int r = 0; r < ROWS; r++) acc += mat[r][c];
    s[c] = acc;
  }
}

// Implement in kernels/vec/colsum_stride_vec.c: sums of columns 0..255 into
// s[0..255] (e.g. two adjacent columns per 64-bit access). Keep the
// column-first traversal. Return 0 when implemented.
__attribute__((weak)) int colsum_stride_vector(int32_t mat[ROWS][COLS], int32_t *s) {
  (void)mat; (void)s;
  return -1;
}

int main(void) {
  fill32(&m[0][0], ROWS * COLS);
  perf_setup();
  MEASURE("colsum_stride", "scalar", colsum_scalar(m, s_ref));
  int rc;
  MEASURE("colsum_stride", "vector", rc = colsum_stride_vector(m, s_vec));
  return kernel_result("colsum_stride", rc == 0, rc == 0 ? compare32(s_ref, s_vec, NCOL) : 0);
}
