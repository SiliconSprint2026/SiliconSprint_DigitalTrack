#!/usr/bin/env python3
"""L1 storage cost model for the bonus track (problem statement Section 8).

Storage per cache = lines x (data bits + tag bits + status bits)
  lines       = size_bytes / line_bytes
  sets        = lines / ways
  tag bits    = PLEN - log2(sets) - log2(line_bytes)      (PLEN = 56, Sv39)
  status bits = 1 valid (+1 dirty for a write-back D-cache)

  tools/cost_model.py --dcache 16 4 128 WB --icache 16 4 128
"""
import argparse
import math

PLEN = 56


def cache_bits(size_kib: int, ways: int, line_bits: int, write_back: bool = False) -> dict:
    size_bytes = size_kib * 1024
    line_bytes = line_bits // 8
    lines = size_bytes // line_bytes
    sets = lines // ways
    tag = PLEN - int(math.log2(sets)) - int(math.log2(line_bytes))
    status = 1 + (1 if write_back else 0)
    data_bits = lines * line_bits
    tag_bits = lines * (tag + status)
    return {"lines": lines, "sets": sets, "tag_width": tag,
            "data_bits": data_bits, "tag_status_bits": tag_bits,
            "total_bits": data_bits + tag_bits}


def total_kib(cfg: dict) -> float:
    d = cache_bits(cfg["dcache_kib"], cfg["dcache_ways"], cfg["line_bits"],
                   cfg.get("dcache_type", "WT") == "WB")
    i = cache_bits(cfg["icache_kib"], cfg["icache_ways"], cfg.get("icache_line_bits", 128))
    return (d["total_bits"] + i["total_bits"]) / 8 / 1024


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--dcache", nargs=4, metavar=("KIB", "WAYS", "LINE_BITS", "WB|WT"),
                    default=["32", "8", "128", "WT"])
    ap.add_argument("--icache", nargs=3, metavar=("KIB", "WAYS", "LINE_BITS"),
                    default=["16", "4", "128"])
    a = ap.parse_args()
    d = cache_bits(int(a.dcache[0]), int(a.dcache[1]), int(a.dcache[2]), a.dcache[3] == "WB")
    i = cache_bits(int(a.icache[0]), int(a.icache[1]), int(a.icache[2]))
    for name, c in (("D-cache", d), ("I-cache", i)):
        print(f"{name}: {c['lines']} lines, {c['sets']} sets, tag {c['tag_width']} b, "
              f"{c['total_bits'] / 8 / 1024:.2f} KiB total")
    print(f"Total L1 storage: {(d['total_bits'] + i['total_bits']) / 8 / 1024:.2f} KiB")


if __name__ == "__main__":
    main()
