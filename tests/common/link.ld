/* SiliconSprint 2026 - shared linker script (Spike and CVA6 testharness).
 * DRAM starts at 0x80000000 on both platforms. */
OUTPUT_ARCH("riscv")
ENTRY(_start)

SECTIONS
{
  . = 0x80000000;
  .text.init : { *(.text.init) }

  . = ALIGN(0x1000);
  .tohost : { *(.tohost) }

  . = ALIGN(0x1000);
  /* Code whose iteration count legitimately differs between Spike and RTL
   * (HTIF polling, the exit loop). tools/trace_compare.py skips this range. */
  .text : {
    __nocompare_start = .;
    *(.text.nocompare)
    __nocompare_end = .;
    *(.text .text.*)
  }

  . = ALIGN(0x1000);
  .rodata : { *(.rodata .rodata.* .srodata .srodata.*) }

  . = ALIGN(0x1000);
  .data : { *(.data .data.*) }
  __global_pointer$ = . + 0x800;
  .sdata : { *(.sdata .sdata.*) }
  .sbss : { *(.sbss .sbss.*) }
  .bss : { *(.bss .bss.*) *(COMMON) }

  . = ALIGN(16);
  _end = .;
  . = . + 0x10000;
  _stack_top = .;
}
