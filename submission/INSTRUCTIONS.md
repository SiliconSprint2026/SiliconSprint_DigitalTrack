# Instruction documentation

## Encoding

Table of every instruction you implemented: mnemonic, funct7, funct3, opcode,
and an example `.insn` line with its 32-bit encoding.

## Semantics

Per instruction: the operation per lane, at each supported SEW. State any
deviation from the problem statement explicitly.

## CSRs

`vcfg` and `vstat`: fields, reset values, read/write behaviour, WARL handling.

## Exceptions

Which encodings raise illegal-instruction exceptions and when (reserved
encodings, `vcfg.EN = 0`). Value of `mtval`.

## Known limitations

Anything not implemented, known bugs, or behaviour you are unsure about.
