// SiliconSprint 2026 - ELF loader DPI helpers for the CVA6 Verilator testharness.
//
// CVA6 v5.3.0 expects these functions from the Spike fork vendored in
// core-v-verif. The frozen environment uses upstream Spike, which does not
// provide them, so they are implemented here (standalone, ELF64 little-endian).
//
//   read_elf(file)            parse PT_LOAD segments of the program
//   get_section(&addr, &len)  iterate over the loaded memory regions
//   read_section_void(a,b,n)  copy region starting at `a` into buffer `b`
//   read_section_sv(a, buf)   same, for an SV open array
//   read_symbol(name, &addr)  look up a symbol (e.g. "tohost")
//
// All segments inside one DRAM window (0x8000_0000 and 0x8400_0000, 64 MiB
// each) are merged into one contiguous, zero-filled region, because the
// testbench preloads exactly the region that starts at the window base.
#include <elf.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

#include "svdpi.h"

namespace {
std::map<uint64_t, std::vector<uint8_t>> g_regions;
std::map<std::string, uint64_t> g_symbols;
std::map<uint64_t, std::vector<uint8_t>>::iterator g_iter;
bool g_iter_valid = false;

constexpr uint64_t kWindowSize = 0x4000000ULL;  // 64 MiB
const uint64_t kWindows[] = {0x80000000ULL, 0x84000000ULL};

uint64_t window_base(uint64_t addr) {
  for (uint64_t w : kWindows)
    if (addr >= w && addr < w + kWindowSize) return w;
  return addr;  // outside DRAM: keep as its own region
}

void place(uint64_t addr, const uint8_t *data, uint64_t filesz, uint64_t memsz) {
  uint64_t base = window_base(addr);
  std::vector<uint8_t> &r = g_regions[base];
  uint64_t off = addr - base;
  if (r.size() < off + memsz) r.resize(off + memsz, 0);
  if (filesz) std::memcpy(r.data() + off, data, filesz);
}
}  // namespace

extern "C" void read_elf(const char *filename) {
  g_regions.clear();
  g_symbols.clear();
  g_iter_valid = false;
  std::ifstream f(filename, std::ios::binary);
  if (!f) {
    std::fprintf(stderr, "[elfloader_dpi] cannot open %s\n", filename);
    return;
  }
  std::vector<uint8_t> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  if (buf.size() < sizeof(Elf64_Ehdr) || std::memcmp(buf.data(), ELFMAG, SELFMAG) != 0 ||
      buf[EI_CLASS] != ELFCLASS64) {
    std::fprintf(stderr, "[elfloader_dpi] %s is not an ELF64 file\n", filename);
    return;
  }
  const Elf64_Ehdr *eh = reinterpret_cast<const Elf64_Ehdr *>(buf.data());
  for (int i = 0; i < eh->e_phnum; i++) {
    const Elf64_Phdr *ph =
        reinterpret_cast<const Elf64_Phdr *>(buf.data() + eh->e_phoff + i * eh->e_phentsize);
    if (ph->p_type != PT_LOAD || ph->p_memsz == 0) continue;
    place(ph->p_paddr, buf.data() + ph->p_offset, ph->p_filesz, ph->p_memsz);
  }
  // symbols
  for (int i = 0; i < eh->e_shnum; i++) {
    const Elf64_Shdr *sh =
        reinterpret_cast<const Elf64_Shdr *>(buf.data() + eh->e_shoff + i * eh->e_shentsize);
    if (sh->sh_type != SHT_SYMTAB) continue;
    const Elf64_Shdr *strh = reinterpret_cast<const Elf64_Shdr *>(
        buf.data() + eh->e_shoff + sh->sh_link * eh->e_shentsize);
    const char *strtab = reinterpret_cast<const char *>(buf.data() + strh->sh_offset);
    const Elf64_Sym *syms = reinterpret_cast<const Elf64_Sym *>(buf.data() + sh->sh_offset);
    for (uint64_t j = 0; j < sh->sh_size / sizeof(Elf64_Sym); j++)
      if (syms[j].st_name) g_symbols[strtab + syms[j].st_name] = syms[j].st_value;
  }
}

extern "C" char get_section(long long *address, long long *len) {
  if (!g_iter_valid) {
    g_iter = g_regions.begin();
    g_iter_valid = true;
  }
  if (g_iter == g_regions.end()) {
    g_iter_valid = false;
    return 0;
  }
  *address = static_cast<long long>(g_iter->first);
  *len = static_cast<long long>(g_iter->second.size());
  ++g_iter;
  return 1;
}

extern "C" void read_section_void(long long address, void *buffer, uint64_t size = 0) {
  auto it = g_regions.find(static_cast<uint64_t>(address));
  if (it == g_regions.end()) return;
  uint64_t n = it->second.size();
  if (size && size < n) n = size;
  std::memcpy(buffer, it->second.data(), n);
}

extern "C" void read_section_sv(long long address, const svOpenArrayHandle buffer) {
  auto it = g_regions.find(static_cast<uint64_t>(address));
  if (it == g_regions.end()) return;
  uint8_t *dst = static_cast<uint8_t *>(svGetArrayPtr(buffer));
  uint64_t n = it->second.size(), cap = static_cast<uint64_t>(svSize(buffer, 1));
  if (dst) std::memcpy(dst, it->second.data(), n < cap ? n : cap);
}

extern "C" char read_symbol(const char *symbol_name, uint64_t *address) {
  auto it = g_symbols.find(symbol_name);
  if (it == g_symbols.end()) return 1;
  *address = it->second;
  return 0;
}
