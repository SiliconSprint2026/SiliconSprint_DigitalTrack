// Kernel: rowsum - repeated row sums of a 64 x 64 int32 matrix (16 KiB).
// The matrix is traversed REPS times, so it is reused: shows the effect of
// cache size and associativity once the working set fits.
#include "kernel_common.h"

#define ROWS 64
#define COLS 64
#define REPS 8
static int32_t m[ROWS][COLS] __attribute__((aligned(64)));
static int32_t y_ref[ROWS], y_vec[ROWS];

static void rowsum_scalar(int32_t mat[ROWS][COLS], int32_t *y) {
  for (int r = 0; r < ROWS; r++) y[r] = 0;
  for (int k = 0; k < REPS; k++)
    for (int r = 0; r < ROWS; r++) {
      int32_t s = 0;
      for (int c = 0; c < COLS; c++) s += mat[r][c];
      y[r] += s;
    }
}

// Implement in kernels/vec/rowsum_vec.c. Same semantics (REPS passes,
// accumulate into y). Return 0 when implemented.
__attribute__((weak)) int rowsum_vector(int32_t mat[ROWS][COLS], int32_t *y) {
  (void)mat; (void)y;
  return -1;
}

int main(void) {
  fill32(&m[0][0], ROWS * COLS);
  perf_setup();
  MEASURE("rowsum", "scalar", rowsum_scalar(m, y_ref));
  int rc;
  MEASURE("rowsum", "vector", rc = rowsum_vector(m, y_vec));
  return kernel_result("rowsum", rc == 0, rc == 0 ? compare32(y_ref, y_vec, ROWS) : 0);
}
