# SiliconSprint 2026 - bare-metal start-up code (Spike and CVA6 testharness).
#
# - clears all integer registers (so RTL and Spike start from the same state)
# - sets up gp, sp, mtvec and mstatus.FS
# - calls main(); its return value becomes the test exit code via tohost
# - traps go to handle_trap() (tests may override it; the default fails the test)

  .section .text.init
  .globl _start
_start:
  li x1, 0
  li x2, 0
  li x3, 0
  li x4, 0
  li x5, 0
  li x6, 0
  li x7, 0
  li x8, 0
  li x9, 0
  li x10, 0
  li x11, 0
  li x12, 0
  li x13, 0
  li x14, 0
  li x15, 0
  li x16, 0
  li x17, 0
  li x18, 0
  li x19, 0
  li x20, 0
  li x21, 0
  li x22, 0
  li x23, 0
  li x24, 0
  li x25, 0
  li x26, 0
  li x27, 0
  li x28, 0
  li x29, 0
  li x30, 0
  li x31, 0

  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop
  la sp, _stack_top

  la t0, trap_entry
  csrw mtvec, t0
  li t0, (1 << 13)          # mstatus.FS = Initial (lets the compiler use FP regs)
  csrs mstatus, t0
  csrwi mcountinhibit, 0    # all counters running

  call main
  j exit

  # exit(code): write (code << 1) | 1 to tohost and spin.
  # Lives in .text.nocompare: the number of spins differs between platforms.
  .section .text.nocompare, "ax", @progbits
  .globl exit
exit:
  slli a0, a0, 1
  ori  a0, a0, 1
  la   t0, tohost
1:
  sd   a0, 0(t0)
  j    1b

  .section .text.init, "ax", @progbits
# ---------------------------------------------------------------------------
# Trap entry: save all GPRs on the stack, call
#   uintptr_t handle_trap(uintptr_t mcause, uintptr_t mepc, uintptr_t mtval,
#                         uintptr_t *regs)
# and resume at the returned PC. regs[i] holds x<i> (regs[0] = 0) and may be
# modified by the handler.
# ---------------------------------------------------------------------------
  .align 4
trap_entry:
  addi sp, sp, -256
  sd x1,   8(sp)
  sd x3,  24(sp)
  sd x4,  32(sp)
  sd x5,  40(sp)
  sd x6,  48(sp)
  sd x7,  56(sp)
  sd x8,  64(sp)
  sd x9,  72(sp)
  sd x10, 80(sp)
  sd x11, 88(sp)
  sd x12, 96(sp)
  sd x13, 104(sp)
  sd x14, 112(sp)
  sd x15, 120(sp)
  sd x16, 128(sp)
  sd x17, 136(sp)
  sd x18, 144(sp)
  sd x19, 152(sp)
  sd x20, 160(sp)
  sd x21, 168(sp)
  sd x22, 176(sp)
  sd x23, 184(sp)
  sd x24, 192(sp)
  sd x25, 200(sp)
  sd x26, 208(sp)
  sd x27, 216(sp)
  sd x28, 224(sp)
  sd x29, 232(sp)
  sd x30, 240(sp)
  sd x31, 248(sp)
  addi t0, sp, 256
  sd t0,  16(sp)            # original sp
  sd x0,   0(sp)

  csrr a0, mcause
  csrr a1, mepc
  csrr a2, mtval
  mv   a3, sp
  call handle_trap
  csrw mepc, a0

  ld x1,   8(sp)
  ld x3,  24(sp)
  ld x4,  32(sp)
  ld x5,  40(sp)
  ld x6,  48(sp)
  ld x7,  56(sp)
  ld x8,  64(sp)
  ld x9,  72(sp)
  ld x10, 80(sp)
  ld x11, 88(sp)
  ld x12, 96(sp)
  ld x13, 104(sp)
  ld x14, 112(sp)
  ld x15, 120(sp)
  ld x16, 128(sp)
  ld x17, 136(sp)
  ld x18, 144(sp)
  ld x19, 152(sp)
  ld x20, 160(sp)
  ld x21, 168(sp)
  ld x22, 176(sp)
  ld x23, 184(sp)
  ld x24, 192(sp)
  ld x25, 200(sp)
  ld x26, 208(sp)
  ld x27, 216(sp)
  ld x28, 224(sp)
  ld x29, 232(sp)
  ld x30, 240(sp)
  ld x31, 248(sp)
  addi sp, sp, 256
  mret

# HTIF mailbox (polled by fesvr in both Spike and the CVA6 testharness)
  .section .tohost, "aw", @progbits
  .align 6
  .globl tohost
tohost: .dword 0
  .align 6
  .globl fromhost
fromhost: .dword 0
