// SiliconSprint 2026 - self-checking test helpers.
//
// A test passes when main() returns 0. Every check prints one line of the form
//   [PASS] <name> expected=0x... actual=0x...
// which is the "expected versus actual" record required in the deliverables.
#ifndef TEST_H
#define TEST_H

#include <stdint.h>

void print_char(char c);
void print_str(const char *s);
void print_hex(uint64_t v);
void print_dec(uint64_t v);

void check64(const char *name, uint64_t got, uint64_t exp);
int test_summary(const char *test_name);

void exit(int code) __attribute__((noreturn));

// Override in a test to handle expected traps. Return the PC to resume at.
uintptr_t handle_trap(uintptr_t mcause, uintptr_t mepc, uintptr_t mtval,
                      uintptr_t *regs);

#define CAUSE_ILLEGAL_INSTRUCTION 2
#define CAUSE_MACHINE_ECALL 11

static inline uint64_t read_mcause(void) {
  uint64_t v;
  asm volatile("csrr %0, mcause" : "=r"(v));
  return v;
}

#endif
