// SiliconSprint 2026 - shared helpers for the bonus-track kernels.
//
// Each kernel <k>.c provides a scalar implementation and calls <k>_vector(),
// which teams implement in kernels/vec/<k>_vec.c using their custom
// instructions. The default (weak) <k>_vector() returns -1 = "not implemented".
#ifndef KERNEL_COMMON_H
#define KERNEL_COMMON_H

#include <stdint.h>
#include "perf.h"
#include "test.h"

static uint32_t lcg_state = 12345;
static inline uint32_t lcg(void) {
  lcg_state = lcg_state * 1664525u + 1013904223u;
  return lcg_state;
}

static inline void fill32(int32_t *p, int n) {
  for (int i = 0; i < n; i++) p[i] = (int32_t)lcg();
}
static inline void fill8(uint8_t *p, int n) {
  for (int i = 0; i < n; i++) p[i] = (uint8_t)(lcg() >> 24);
}

static inline int compare32(const int32_t *x, const int32_t *y, int n) {
  for (int i = 0; i < n; i++)
    if (x[i] != y[i]) return i + 1;
  return 0;
}
static inline int compare8(const uint8_t *x, const uint8_t *y, int n) {
  for (int i = 0; i < n; i++)
    if (x[i] != y[i]) return i + 1;
  return 0;
}

// Runs one measured region and prints its PERF line.
#define MEASURE(kernel, variant, stmt)  \
  do {                                  \
    perf_t _s, _d;                      \
    perf_begin(&_s);                    \
    stmt;                               \
    perf_end(&_s, &_d);                 \
    perf_report(kernel, variant, &_d);  \
  } while (0)

// Reports the vector-vs-scalar result and returns the test exit code.
static inline int kernel_result(const char *kernel, int implemented, int mismatch) {
  if (!implemented) {
    print_str("KERNEL ");
    print_str(kernel);
    print_str(" vector=not_implemented\n");
    return 0;
  }
  print_str("KERNEL ");
  print_str(kernel);
  print_str(mismatch ? " vector=MISMATCH\n" : " vector=match\n");
  return mismatch ? 1 : 0;
}

#endif
