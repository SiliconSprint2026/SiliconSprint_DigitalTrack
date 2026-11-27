// SiliconSprint 2026 - performance measurement helpers (bonus track).
//
// Uses mcycle, minstret and CVA6 hardware performance counters 3..6.
// CVA6 event IDs (core/perf_counters.sv):
//   0x01 L1 I-cache miss   0x02 L1 D-cache miss
//   0x10 L1 I-cache access 0x11 L1 D-cache access
//
// Place perf_begin()/perf_end() around the region of interest ONLY.
// On Spike these counters read as 0 (no timing model); compare results there,
// not cycles. Trace comparison automatically ignores counter reads.
#ifndef PERF_H
#define PERF_H

#include <stdint.h>
#include "test.h"

#define EV_ICACHE_MISS   0x01
#define EV_DCACHE_MISS   0x02
#define EV_ICACHE_ACCESS 0x10
#define EV_DCACHE_ACCESS 0x11

typedef struct {
  uint64_t cycles, instret, icache_miss, dcache_miss, icache_access, dcache_access;
} perf_t;

#define PERF_CSR_READ(csr) ({ uint64_t _v; asm volatile("csrr %0, " #csr : "=r"(_v)); _v; })

static inline void perf_setup(void) {
  asm volatile("csrw mhpmevent3, %0" ::"r"((uint64_t)EV_ICACHE_MISS));
  asm volatile("csrw mhpmevent4, %0" ::"r"((uint64_t)EV_DCACHE_MISS));
  asm volatile("csrw mhpmevent5, %0" ::"r"((uint64_t)EV_ICACHE_ACCESS));
  asm volatile("csrw mhpmevent6, %0" ::"r"((uint64_t)EV_DCACHE_ACCESS));
}

static inline void perf_snapshot(perf_t *p) {
  asm volatile("fence" ::: "memory");
  p->cycles = PERF_CSR_READ(mcycle);
  p->instret = PERF_CSR_READ(minstret);
  p->icache_miss = PERF_CSR_READ(mhpmcounter3);
  p->dcache_miss = PERF_CSR_READ(mhpmcounter4);
  p->icache_access = PERF_CSR_READ(mhpmcounter5);
  p->dcache_access = PERF_CSR_READ(mhpmcounter6);
}

static inline void perf_begin(perf_t *start) { perf_snapshot(start); }

static inline void perf_end(perf_t *start, perf_t *delta) {
  perf_t now;
  perf_snapshot(&now);
  delta->cycles = now.cycles - start->cycles;
  delta->instret = now.instret - start->instret;
  delta->icache_miss = now.icache_miss - start->icache_miss;
  delta->dcache_miss = now.dcache_miss - start->dcache_miss;
  delta->icache_access = now.icache_access - start->icache_access;
  delta->dcache_access = now.dcache_access - start->dcache_access;
}

// Prints one machine-readable line, parsed by tools/cache_sweep.py:
// PERF kernel=<k> variant=<scalar|vector> cycles=... instret=... ...
static inline void perf_report(const char *kernel, const char *variant, const perf_t *d) {
  print_str("PERF kernel=");
  print_str(kernel);
  print_str(" variant=");
  print_str(variant);
  print_str(" cycles=");
  print_dec(d->cycles);
  print_str(" instret=");
  print_dec(d->instret);
  print_str(" icache_miss=");
  print_dec(d->icache_miss);
  print_str(" dcache_miss=");
  print_dec(d->dcache_miss);
  print_str(" icache_access=");
  print_dec(d->icache_access);
  print_str(" dcache_access=");
  print_dec(d->dcache_access);
  print_char('\n');
}

#endif
