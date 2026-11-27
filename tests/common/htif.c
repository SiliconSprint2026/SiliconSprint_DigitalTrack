// SiliconSprint 2026 - minimal console and trap support over HTIF.
// Works identically on Spike and on the CVA6 Verilator testharness (both use
// riscv-fesvr): printing is a SYS_write proxied through tohost/fromhost.
#include <stdint.h>
#include "test.h"

extern volatile uint64_t tohost;
extern volatile uint64_t fromhost;

#define SYS_write 64

// In .text.nocompare: the polling loop runs a platform-dependent number of
// times, so trace comparison ignores it.
__attribute__((noinline, section(".text.nocompare")))
static void htif_syscall(uint64_t n, uint64_t a0, uint64_t a1, uint64_t a2) {
  static volatile uint64_t magic_mem[8] __attribute__((aligned(64)));
  magic_mem[0] = n;
  magic_mem[1] = a0;
  magic_mem[2] = a1;
  magic_mem[3] = a2;
  __sync_synchronize();
  tohost = (uint64_t)(uintptr_t)magic_mem;
  while (fromhost == 0) {
  }
  fromhost = 0;
  __sync_synchronize();
}

// Line-buffered output: one syscall per line keeps simulation fast.
static char line_buf[256];
static unsigned line_len;

static void flush_line(void) {
  if (line_len) {
    htif_syscall(SYS_write, 1, (uint64_t)(uintptr_t)line_buf, line_len);
    line_len = 0;
  }
}

void print_char(char c) {
  line_buf[line_len++] = c;
  if (c == '\n' || line_len == sizeof(line_buf)) flush_line();
}

void print_str(const char *s) {
  while (*s) print_char(*s++);
}

void print_hex(uint64_t v) {
  static const char digits[] = "0123456789abcdef";
  print_str("0x");
  for (int i = 60; i >= 0; i -= 4) print_char(digits[(v >> i) & 0xf]);
}

void print_dec(uint64_t v) {
  char tmp[24];
  int n = 0;
  do {
    tmp[n++] = (char)('0' + v % 10);
    v /= 10;
  } while (v);
  while (n) print_char(tmp[--n]);
}

// ---------------------------------------------------------------------------
// Self-checking helpers
// ---------------------------------------------------------------------------
static unsigned n_pass, n_fail;

void check64(const char *name, uint64_t got, uint64_t exp) {
  if (got == exp) {
    n_pass++;
    print_str("[PASS] ");
  } else {
    n_fail++;
    print_str("[FAIL] ");
  }
  print_str(name);
  print_str(" expected=");
  print_hex(exp);
  print_str(" actual=");
  print_hex(got);
  print_char('\n');
}

int test_summary(const char *test_name) {
  print_str("[SUMMARY] ");
  print_str(test_name);
  print_str(" pass=");
  print_dec(n_pass);
  print_str(" fail=");
  print_dec(n_fail);
  print_char('\n');
  return n_fail ? 1 : 0;
}

// Default trap handler: any trap is a test failure (exit code 0x100 + mcause).
// Tests that expect traps provide their own handle_trap().
__attribute__((weak)) uintptr_t handle_trap(uintptr_t mcause, uintptr_t mepc,
                                            uintptr_t mtval, uintptr_t *regs) {
  (void)regs;
  print_str("[TRAP] unexpected trap mcause=");
  print_hex(mcause);
  print_str(" mepc=");
  print_hex(mepc);
  print_str(" mtval=");
  print_hex(mtval);
  print_char('\n');
  exit(0x100 + (int)(mcause & 0xff));
  return mepc;
}
