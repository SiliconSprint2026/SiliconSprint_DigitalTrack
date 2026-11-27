# Bonus track: memory-system design exploration

Goal: choose the L1 cache configuration that gives the best performance on the
kernels **within the storage budget**, and justify it with measured data.
Graded on reasoning, not on the lowest cycle count.

## Frozen parameters

| Parameter | Value |
| --- | --- |
| Main-memory latency | **40 cycles** per AXI request (`MEM_LATENCY`, set by `tools/cache_sweep.py`) |
| L1 storage budget (I + D, incl. tag and status bits) | **40 KiB** |

These values are fixed for the event. Results produced with other values are
not valid.

## What you may change

Only cache **parameters**, through `tools/cache_sweep.py`:

| Parameter | Allowed values |
| --- | --- |
| D-cache size | 4 / 8 / 16 / 32 KiB |
| D-cache associativity | 1 / 2 / 4 / 8 (1-way only with WT: a direct-mapped write-back cache is broken in CVA6 v5.3.0) |
| Line width | 128 / 256 / 512 bits |
| D-cache type | `WT` (write-through, `wt_dcache`) or `WB` (write-back, `std_nbdcache`) |
| Write-buffer depth (WT only) | 2 / 4 / 8 |
| I-cache size / associativity | 4 / 8 / 16 KiB, 2 / 4 ways |

Pre-validated configurations are listed in `tools/cache_configs.json`. Any other
combination must be validated by the organizers before you rely on it.

## Workflow

```bash
# 1. implement the vector kernels in tests/kernels/vec/*_vec.c (#if 0 -> #if 1)
make -C tests

# 2. sweep (each configuration = one Verilator build, ~5-10 min)
python3 tools/cache_sweep.py --configs base,d16w4_wt,d16w4_wb,d8w2_wt

# 3. results
sweep_out/results.csv      # one row per config x kernel x variant
sweep_out/<kernel>.png     # cycles and D-cache miss rate per configuration

# 4. cost of a configuration
python3 tools/cost_model.py --dcache 16 4 128 WB --icache 8 4 128
```

`results.csv` columns: config, kernel, variant (scalar / vector), cycles,
instret, icache_miss, dcache_miss, icache_access, dcache_access, ipc,
dcache_miss_rate, l1_kib, within_budget, vector_check.

## The kernels

| Kernel | Pattern | Tells you about |
| --- | --- | --- |
| `vadd_stream` | 3 × 16 KiB streams, no reuse | Line size; cache size barely matters |
| `blend8` | 32 KiB in-place read-modify-write (+32 KiB deltas), SEW = 8 | Write-back vs write-through traffic |
| `rowsum` | 16 KiB matrix read 8 times | Capacity and reuse |
| `colsum_stride` | 32 rows × 256 columns, 4 KiB row stride (power of two) | Conflict misses, associativity |
| `dot` | Two read-only streams (needs VMUL / VMAC) | Bandwidth-bound reduction |

## Things to notice and explain

* Which kernels are memory-bound and which are compute-bound, for scalar and for vector code.
* Why vector code is **more** sensitive to the memory system than scalar code.
* CVA6's `WT` and `WB` data caches are different implementations (different
  miss handling and write buffering), not one cache with a policy switch.
  Say what that means for your comparison.
* The fixed memory latency is a simplification of a real memory system.
