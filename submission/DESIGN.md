# Architecture and design decisions

## Block diagram

Add a diagram image to `submission/` (e.g. `block_diagram.png`) and embed it here with
`![block diagram](block_diagram.png)`. It must show the VALU and where it connects to the
CVA6 pipeline: decoder, issue / scoreboard, execute, writeback, commit, CSR file.

## Execution flow

Follow one VADD from fetch to commit. Name the modules and signals it passes
through and the cycle in which each step happens.

## Files changed

| File | Change |
| --- | --- |
| `cva6/core/...` | |

## Design decisions

Explain each choice and the alternative you rejected:

1. **Functional unit and writeback port:** new writeback port or shared? Why?
2. **Datapath:** how lanes and SEW are implemented (e.g. carry-break adder); area/timing considerations.
3. **Structural hazards:** when is your unit busy, and how does the issue stage know?
4. **`vcfg` ordering (rule 5.4.1):** how a vector instruction after a `vcfg` write sees the new value.
5. **Precise `vstat` (rule 5.4.2):** how you guarantee only committed instructions set OV.
6. **Precise exceptions (rule 5.4.3):** where the illegal-instruction check happens.
7. **Stretch goals:** pipelining, flush handling, third operand (if implemented).

## What we would do with more time
