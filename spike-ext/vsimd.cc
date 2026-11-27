// SiliconSprint 2026 - Spike golden model for the custom packed-SIMD extension.
//
// PROVIDED VERSION: models the five MANDATORY instructions at SEW = 32 only.
//
// Stage 2 task F.1 - teams must extend this model with:
//   * SEW = 8 and 16 (element width taken from vcfg.SEW)
//   * the vcfg (0x8C0) and vstat (0x8C1) CSRs        -> get_csrs()
//   * the illegal-instruction trap when vcfg.EN = 0
//   * vstat.OV on signed overflow of VADD / VSUB
//   * any stretch instructions the team implements (VSLL, VSRL, VMUL, VMAC)
// Look for "TODO(team)" below.
//
// Build:  make -C spike-ext            -> spike-ext/build/libvsimd.so
// Use:    spike --isa=rv64gc --extlib=spike-ext/build/libvsimd.so --extension=vsimd <elf>
//
// Note: Spike resets the hart before --extension libraries are loaded, so
// custom CSRs returned by get_csrs() must also be added to the hart's CSR map
// when get_instructions() is called (see processor_t::get_state()->add_csr).
#include "extension.h"
#include "processor.h"
#include "trap.h"
#include "disasm.h"

#include <cstdint>
#include <vector>

namespace {

// R-type encoding fields, custom-0 opcode.
constexpr uint32_t kOpcode = 0x0B;
constexpr uint32_t kMask = 0xFE00707F;  // funct7 | funct3 | opcode
constexpr uint32_t enc(uint32_t funct3, uint32_t funct7) {
  return (funct7 << 25) | (funct3 << 12) | kOpcode;
}

enum class Op { ADD, SUB, AND, OR, XOR };

// ---------------------------------------------------------------------------
// Register access helpers (work in both fast and commit-logging modes).
// ---------------------------------------------------------------------------
inline reg_t xreg(processor_t *p, unsigned r) { return p->get_state()->XPR[r]; }

inline void write_rd(processor_t *p, insn_t insn, reg_t value) {
  state_t *s = p->get_state();
  if (p->get_log_commits_enabled()) s->log_reg_write[insn.rd() << 4] = {value, 0};
  s->XPR.write(insn.rd(), value);
}

// ---------------------------------------------------------------------------
// Lane arithmetic. TODO(team): generalise to SEW = 8 / 16 / 32 using vcfg.
// ---------------------------------------------------------------------------
uint64_t lanes32(Op op, uint64_t a, uint64_t b) {
  uint64_t result = 0;
  for (int lane = 0; lane < 2; lane++) {
    uint32_t x = static_cast<uint32_t>(a >> (32 * lane));
    uint32_t y = static_cast<uint32_t>(b >> (32 * lane));
    uint32_t r = 0;
    switch (op) {
      case Op::ADD: r = x + y; break;
      case Op::SUB: r = x - y; break;
      case Op::AND: r = x & y; break;
      case Op::OR:  r = x | y; break;
      case Op::XOR: r = x ^ y; break;
    }
    result |= static_cast<uint64_t>(r) << (32 * lane);
  }
  return result;
}

template <Op OP>
reg_t exec(processor_t *p, insn_t insn, reg_t pc) {
  // TODO(team): if vcfg.EN == 0 -> throw trap_illegal_instruction(insn.bits());
  uint64_t r = lanes32(OP, xreg(p, insn.rs1()), xreg(p, insn.rs2()));
  // TODO(team): set vstat.OV on signed overflow (VADD / VSUB only).
  write_rd(p, insn, r);
  return pc + 4;
}

// Disassembly argument printers.
struct : public arg_t {
  std::string to_string(insn_t insn) const override { return xpr_name[insn.rd()]; }
} arg_rd;
struct : public arg_t {
  std::string to_string(insn_t insn) const override { return xpr_name[insn.rs1()]; }
} arg_rs1;
struct : public arg_t {
  std::string to_string(insn_t insn) const override { return xpr_name[insn.rs2()]; }
} arg_rs2;

#define VSIMD_INSN(f3, f7, fn) {enc(f3, f7), kMask, fn, fn, fn, fn, fn, fn, fn, fn}

class vsimd_t : public extension_t {
 public:
  const char *name() const override { return "vsimd"; }

  std::vector<insn_desc_t> get_instructions(const processor_t &) override {
    return {
        VSIMD_INSN(0, 0, exec<Op::ADD>),
        VSIMD_INSN(1, 0, exec<Op::SUB>),
        VSIMD_INSN(2, 0, exec<Op::AND>),
        VSIMD_INSN(3, 0, exec<Op::OR>),
        VSIMD_INSN(4, 0, exec<Op::XOR>),
        // TODO(team): stretch instructions VSLL (5,0), VSRL (6,0), VMUL (7,0),
        //             VMAC (0,1). Any encoding not registered here is illegal.
    };
  }

  std::vector<disasm_insn_t *> get_disasms(const processor_t *) override {
    return {
        new disasm_insn_t("vadd", enc(0, 0), kMask, {&arg_rd, &arg_rs1, &arg_rs2}),
        new disasm_insn_t("vsub", enc(1, 0), kMask, {&arg_rd, &arg_rs1, &arg_rs2}),
        new disasm_insn_t("vand", enc(2, 0), kMask, {&arg_rd, &arg_rs1, &arg_rs2}),
        new disasm_insn_t("vor", enc(3, 0), kMask, {&arg_rd, &arg_rs1, &arg_rs2}),
        new disasm_insn_t("vxor", enc(4, 0), kMask, {&arg_rd, &arg_rs1, &arg_rs2}),
    };
  }

  // TODO(team): return the vcfg / vstat CSR objects here (subclass csr_t from
  // csrs.h: implement read() and unlogged_write()). Create them once and keep
  // them across calls, and also register them from get_instructions().
  std::vector<csr_t_p> get_csrs(processor_t &) const override { return {}; }
};

}  // namespace

REGISTER_EXTENSION(vsimd, []() { static vsimd_t ext; return &ext; })
