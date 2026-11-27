# CVA6 orientation guide

This guide tells you **where to look** in the frozen CVA6 v5.3.0 source and
**which questions to answer**. It does not tell you how to implement the
challenge. Official documentation: <https://docs.openhwgroup.org/projects/cva6-user-manual/>.

## Pipeline at a glance

```
 PC gen -> IF -> ID (decode) -> Issue (scoreboard, read operands) -> EX (functional units) -> Commit
                    |                     ^           |                         |               |
                    |                     |  forwarding / writeback ports <-----+               |
                    +--- exceptions ------+-------------------------------------> in-order commit, CSRs, flush
```

* **In-order issue**: one instruction per cycle in `cv64a6_imafdc_sv39`.
* **Out-of-order completion**: functional units have different latencies and
  write results to the scoreboard through several **writeback ports**.
* **In-order commit**: up to two instructions per cycle retire in program order.
* **Speculation**: instructions after a predicted branch are issued and
  executed. On a mispredict or exception they are flushed from the scoreboard.

## Files you will read

| File (`cva6/core/`) | What it does | Questions to answer |
| --- | --- | --- |
| `include/riscv_pkg.sv` | Opcodes, instruction formats, CSR addresses | Where is `OpcodeCustom0` defined? |
| `include/ariane_pkg.sv` | `fu_t` (functional-unit types), `fu_op` (operations), writeback-port indices (`FLU_WB`, …) | How many bits does `fu_t` have? Which units share `FLU_WB`? |
| `include/cv64a6_imafdc_sv39_config_pkg.sv` | The frozen configuration | Do not change it (except via the cache sweep in the bonus track) |
| `decoder.sv` | Turns a 32-bit instruction into a `scoreboard_entry_t` | How does an unknown opcode become an illegal-instruction exception? |
| `id_stage.sv` | Wraps the decoders | Which signals from the CSR file reach the decoder? |
| `issue_stage.sv`, `issue_read_operands.sv` | Operand read and forwarding, structural hazards (`fus_busy`), per-FU valid signals | When is the ALU considered busy, and why does that depend on the multiplier? |
| `scoreboard.sv` | Tracks in-flight instructions, collects writeback, provides forwarding | What does the scoreboard store when a unit writes back? What happens to `ex.cause` when `ex.valid` is 0? |
| `ex_stage.sv` | Instantiates ALU, branch unit, CSR buffer, multiplier (the fixed-latency path), LSU, FPU | How is `flu_result_o` selected? Who drives `flu_exception_o`? |
| `alu.sv`, `mult.sv` | Reference single-cycle and multi-cycle units | How does `mult.sv` handle `flush_i`? |
| `commit_stage.sv` | Retires instructions, writes registers and CSRs | Which unit types may commit on the second port? How do FPU flags reach `fflags`? |
| `csr_regfile.sv` | All CSRs, privilege checks, `flush_o` on CSR side effects | Which CSR writes flush the pipeline, and why? |
| `controller.sv` | Turns flush requests into pipeline flushes | What gets flushed on a CSR side effect vs a mispredict? |
| `cva6.sv` | Top level: wires every stage together | Every new port must be connected here |

## Things that trip people up

* **Ports must be connected everywhere.** The build uses `-Werror-PINMISSING`.
* **New files** must be added to `core/Flist.cva6`, or Verilator will not see them.
* **x0 destination.** Instructions with `rd = x0` still commit. Their result is simply not written.
* **The second commit port** only retires certain unit types. A new unit type
  works without being added there, but commits at most one per cycle.
* **Speculation and state.** Anything your unit changes outside the register
  file (for example a status flag) must not change for an instruction that is
  later flushed.

## Debugging

* `run_out/<test>/trace_rvfi_hart_00.dasm` lists every committed instruction
  and the register it wrote. Compare it with Spike's log
  (`scripts/run_test.sh cmp <elf>` does this for you).
* For waveforms, build with `TRACE=1 scripts/build_rtl.sh` and run
  `scripts/run_test.sh wave <elf>`. Useful signals:
  `i_ariane.i_cva6.id_stage_i.decoder_i.instruction_o`,
  `i_ariane.i_cva6.issue_stage_i.i_issue_read_operands.*`,
  `i_ariane.i_cva6.ex_stage_i.flu_*`, `i_ariane.i_cva6.commit_stage_i.commit_ack_o`.
