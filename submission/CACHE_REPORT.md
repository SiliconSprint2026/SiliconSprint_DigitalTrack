# Bonus track: memory-system exploration

Put the raw `sweep_out/results.csv` and plots in `submission/bonus/`.

## Configurations measured

| Config | D$ size / ways / line / type | I$ size / ways | L1 KiB (cost model) | Within budget |
| --- | --- | --- | --- | --- |
| | | | | |

## Results

Key numbers per kernel (scalar and vector cycles, miss rates). Reference the plots.

## Analysis

* Which kernels are memory-bound and which are compute-bound (scalar vs vector)?
* Effect of line size, associativity and write policy.
* Why vector code is more sensitive to the memory system.

## Final choice

The configuration you pick, its cost under the budget, and the trade-offs it gives up.

## Limitations of the method

E.g. CVA6's WT and WB caches are different implementations; fixed memory latency.
