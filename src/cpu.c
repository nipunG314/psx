#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "log.h"
#include "interconnect.h"
#include "instruction.h"
#include "flag.h"

void log_ins(Ins ins) {
  uint32_t func = get_func(ins);
  log_trace("ins_func: 0x%08X %s", func, funcs[func]);
  if (func == 0x0) {
    uint32_t sub_func = get_sub_func(ins);
    log_trace("ins_sub_func: 0x%08X %s", sub_func, special_funcs[sub_func]);
  }
  if (func == 0x10) {
    log_trace("ins_cop_func: 0x%08X", get_cop_func(ins));
  }
  log_trace("instruction: 0x%08X", ins.data);
}

Cpu init_cpu(char const *bios_filename) {
  Cpu cpu = {0};

  cpu.pc = MAKE_Addr(0xBFC00000);
  cpu.next_pc = MAKE_Addr(cpu.pc.data + 4);
  cpu.current_pc = MAKE_Addr(0x0);
  for(int index = 0; index < 32; index++) {
    cpu.regs[index] = 0xDEADDEAD;
    cpu.output_regs[index] = 0xDEADDEAD;
  }
  cpu.regs[0] = 0x0;
  cpu.sr = 0x0;
  cpu.cause = 0x0;
  cpu.epc = MAKE_Addr(0x0);
  cpu.hi = cpu.lo = 0xDEADDEAD;
  cpu.load_delay_slot = MAKE_LoadDelaySlot(MAKE_RegIndex(0x0), 0x0); 
  cpu.inter = init_interconnect(bios_filename);

  return cpu;
}

uint32_t load32(Cpu *cpu, Addr addr) {
  return load_inter32(&cpu->inter, addr);
}

uint8_t load8(Cpu *cpu, Addr addr) {
  return load_inter8(&cpu->inter, addr);
}

void store32(Cpu *cpu, Addr addr, uint32_t val) {
  store_inter32(&cpu->inter, addr, val);
}

void store16(Cpu *cpu, Addr addr, uint16_t val) {
  store_inter16(&cpu->inter, addr, val);
}

void store8(Cpu *cpu, Addr addr, uint8_t val) {
  store_inter8(&cpu->inter, addr, val);
}

void set_reg(Cpu *cpu, RegIndex index, uint32_t value) {
  cpu->output_regs[index.data] = value;
  cpu->output_regs[0] = 0x0;
}

void run_next_ins(Cpu *cpu) {
  if (get_flag(PRINT_PC))
    log_trace("PC: 0x%08X", cpu->pc);

  // Fetch the instruction
  Ins ins = MAKE_Ins(load32(cpu, cpu->pc));
  cpu->current_pc = cpu->pc;

  if (get_flag(PRINT_INS))
    log_ins(ins);
  if (get_flag(OUTPUT_LOG))
    printf("%08x %08x\n", cpu->pc.data, ins.data);

  // Increment PC
  cpu->pc = cpu->next_pc;
  cpu->next_pc = MAKE_Addr(cpu->next_pc.data + 4);

  // Update out_regs as per load_delay_slot
  set_reg(cpu, cpu->load_delay_slot.index, cpu->load_delay_slot.val);
  cpu->load_delay_slot = MAKE_LoadDelaySlot(MAKE_RegIndex(0x0), 0x0);

  // Execute current instruction
  decode_and_execute(cpu, ins);

  // Copy output_regs into regs
  memcpy(cpu->regs, cpu->output_regs, sizeof cpu->regs);
}

void exception(Cpu *cpu, Exception exp) {
  Addr handler_addr = MAKE_Addr((cpu->sr & (1 << 22)) ? 0xBFC00180 : 0x80000080);

  uint32_t mode = cpu->sr & 0x3F;
  cpu->sr = cpu->sr & ~0x3F;
  cpu->sr = cpu->sr | ((mode << 2) & 0x3F);

  cpu->cause = exp << 2;
  cpu->epc = cpu->current_pc;

  cpu->pc = handler_addr;
  cpu->next_pc = MAKE_Addr(cpu->pc.data + 4);
}

void branch(Cpu *cpu, uint32_t offset) {
  offset = offset << 2;

  cpu->next_pc = MAKE_Addr(cpu->pc.data + offset);
}

void op_lui(Cpu *cpu, Ins ins) {
  uint32_t imm = get_imm(ins);
  RegIndex rt = get_rt(ins);

  set_reg(cpu, rt, imm << 16);
}

void op_ori(Cpu *cpu, Ins ins) {
  uint32_t imm = get_imm(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);

  set_reg(cpu, rt, imm | cpu->regs[rs.data]);
}

void op_andi(Cpu *cpu, Ins ins) {
  uint32_t imm = get_imm(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);

  set_reg(cpu, rt, imm & cpu->regs[rs.data]);
}

void op_sw(Cpu *cpu, Ins ins) {
  if (cpu->sr & 0x10000) {
    log_info("Ignoring store32 calls while cache is isolated");
    return;
  }

  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);

  store32(cpu, MAKE_Addr(imm_se + cpu->regs[rs.data]), cpu->regs[rt.data]);
}

void op_sll(Cpu *cpu, Ins ins) {
  uint32_t shift = get_shift(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);

  set_reg(cpu, rd, cpu->regs[rt.data] << shift);
}

void op_addiu(Cpu *cpu, Ins ins) {
  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);

  set_reg(cpu, rt, cpu->regs[rs.data] + imm_se);
}

void op_addi(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  int32_t imm_se = get_imm_se(ins);
  int32_t reg_s = cpu->regs[rs.data];

  if (reg_s >= 0 && imm_se > INT32_MAX - reg_s) {
    fatal("Unhandled Exception: Signed overflow");  
  }

  set_reg(cpu, rt, imm_se + reg_s);
}

void op_j(Cpu *cpu, Ins ins) {
  uint32_t imm_jump = get_imm_jump(ins);

  cpu->next_pc = MAKE_Addr((cpu->next_pc.data & 0xF0000000) | (imm_jump << 2));
}

void op_jal(Cpu *cpu, Ins ins) {
  set_reg(cpu, MAKE_RegIndex(0x1F), cpu->next_pc.data);

  op_j(cpu, ins);
}

void op_jalr(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rd = get_rd(ins);

  set_reg(cpu, rd, cpu->next_pc.data);
  cpu->next_pc = MAKE_Addr(cpu->regs[rs.data]);
}

void op_or(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);

  set_reg(cpu, rd, cpu->regs[rs.data] | cpu->regs[rt.data]);
}

void op_and(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);

  set_reg(cpu, rd, cpu->regs[rs.data] & cpu->regs[rt.data]);
}

void op_mtc0(Cpu *cpu, Ins ins) {
  RegIndex rt = get_rt(ins);
  uint8_t cop_reg = get_cop_reg(ins);
  uint32_t val = cpu->regs[rt.data];

  switch (cop_reg) {
    case 3:
    case 5:
    case 6:
    case 7:
    case 9:
    case 11:
      if (val != 0) {
        fatal("Unhandled write to cop0 register. RegIndex: %d, Val: %d", cop_reg, val);
      }
      break;
    case 12:
      cpu->sr = val;
      break;
    case 13:
      if (val != 0) {
        fatal("Unhandled write to CAUSE register. Val: %d", val);
      }
      break;
    default:
      fatal("Unhandled write to cop0 register. RegIndex: %d", cop_reg);
  }
}

void op_mfc0(Cpu *cpu, Ins ins) {
  RegIndex rt = get_rt(ins);
  uint8_t cop_reg = get_cop_reg(ins);

  switch (cop_reg) {
    case 12:
      cpu->load_delay_slot = MAKE_LoadDelaySlot(rt, cpu->sr);
      break;
    case 13:
      cpu->load_delay_slot = MAKE_LoadDelaySlot(rt, cpu->cause);
      break;
    case 14:
      cpu->load_delay_slot = MAKE_LoadDelaySlot(rt, cpu->epc.data);
      break;
    default:
      fatal("Unhandled read from cop0 register. RegIndex: %d", cop_reg);
  }
}

void op_rfe(Cpu *cpu, Ins ins) {
  if ((ins.data & 0x3F) != 0x10)
    fatal("Invalid cop0 instruction: 0x%08X", ins);

  uint32_t mode = cpu->sr & 0x3F;
  cpu->sr = cpu->sr & ~0x3F;
  cpu->sr = cpu->sr | (mode >> 2);
}

void op_beq(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  uint32_t imm_se = get_imm_se(ins);

  if (cpu->regs[rs.data] == cpu->regs[rt.data])
    branch(cpu, imm_se);
}

void op_bne(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  uint32_t imm_se = get_imm_se(ins);

  if (cpu->regs[rs.data] != cpu->regs[rt.data])
    branch(cpu, imm_se);
}

void op_blez(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  uint32_t imm_se = get_imm_se(ins);

  int32_t reg_s = cpu->regs[rs.data];

  if (reg_s <= 0)
    branch(cpu, imm_se);
}

void op_bgtz(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  uint32_t imm_se = get_imm_se(ins);

  int32_t reg_s = cpu->regs[rs.data];

  if (reg_s > 0)
    branch(cpu, imm_se);
}

void op_lw(Cpu *cpu, Ins ins) {
  if (cpu->sr & 0x10000) {
    log_info("Ignoring load32 calls while cache is isolated");
    return;
  }

  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);

  cpu->load_delay_slot = MAKE_LoadDelaySlot(rt, load32(cpu, MAKE_Addr(cpu->regs[rs.data] + imm_se)));
}

void op_lb(Cpu *cpu, Ins ins) {
  if (cpu->sr & 0x10000) {
    log_info("Ignoring load8 calls while cache is isolated");
    return;
  }

  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);

  int8_t data = load8(cpu, MAKE_Addr(cpu->regs[rs.data] + imm_se));
  cpu->load_delay_slot = MAKE_LoadDelaySlot(rt, data);
}

void op_lbu(Cpu *cpu, Ins ins) {
  if (cpu->sr & 0x10000) {
    log_info("Ignoring load8 calls while cache is isolated");
    return;
  }

  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);

  cpu->load_delay_slot = MAKE_LoadDelaySlot(rt, load8(cpu, MAKE_Addr(cpu->regs[rs.data] + imm_se)));
}

void op_sltu(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);

  set_reg(cpu, rd, cpu->regs[rs.data] < cpu->regs[rt.data]);
}

void op_slt(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);
  int32_t reg_s = cpu->regs[rs.data];
  int32_t reg_t = cpu->regs[rt.data];

  set_reg(cpu, rd, reg_s < reg_t);
}

void op_slti(Cpu *cpu, Ins ins) {
  int32_t imm_se = get_imm_se(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  int32_t reg_s = cpu->regs[rs.data];

  set_reg(cpu, rt, reg_s < imm_se);
}

void op_sltiu(Cpu *cpu, Ins ins) {
  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);

  set_reg(cpu, rt, cpu->regs[rs.data] < imm_se);
}

void op_add(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);
  int32_t reg_s = cpu->regs[rs.data];
  int32_t reg_t = cpu->regs[rt.data];

  if (reg_s >= 0 && reg_s > INT32_MAX - reg_t)
    fatal("Unhandled Exception: Signed Overflow");

  set_reg(cpu, rd, reg_s + reg_t);
}

void op_addu(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);

  set_reg(cpu, rd, cpu->regs[rs.data] + cpu->regs[rt.data]);
}

void op_sh(Cpu *cpu, Ins ins) {
  if (cpu->sr & 0x10000) {
    log_info("Ignoring store16 calls while cache is isolated");
    return;
  }

  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  uint32_t imm_se = get_imm_se(ins);

  store16(cpu, MAKE_Addr(cpu->regs[rs.data] + imm_se), cpu->regs[rt.data]);
}

void op_sb(Cpu *cpu, Ins ins) {
  if (cpu->sr & 0x10000) {
    log_info("Ignoring store16 calls while cache is isolated");
    return;
  }

  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  uint32_t imm_se = get_imm_se(ins);

  store8(cpu, MAKE_Addr(cpu->regs[rs.data] + imm_se), cpu->regs[rt.data]);
}

void op_jr(Cpu *cpu, Ins ins) {
  cpu->next_pc = MAKE_Addr(cpu->regs[get_rs(ins).data]);
}

void op_bxx(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  uint32_t imm_se = get_imm_se(ins);

  int is_bgez = (ins.data >> 16) & 0x1 ? 1 : 0;
  int is_link = (((ins.data >> 17) & 0xf) == 0x8) ? 1 : 0;

  int32_t reg_s = cpu->regs[rs.data];
  uint32_t test = (reg_s < 0);
  test = test ^ is_bgez;

  if (is_link)
    set_reg(cpu, MAKE_RegIndex(0x1F), cpu->next_pc.data);

  if (test != 0) {
    branch(cpu, imm_se);
  }
}

void op_subu(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);

  set_reg(cpu, rd, cpu->regs[rs.data] - cpu->regs[rt.data]);
}

void op_sra(Cpu *cpu, Ins ins) {
  uint32_t shift = get_shift(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);

  int32_t reg_t = cpu->regs[rt.data];
  reg_t = reg_t >> shift;

  set_reg(cpu, rd, reg_t);
}

void op_srl(Cpu *cpu, Ins ins) {
  uint32_t shift = get_shift(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);

  set_reg(cpu, rd, cpu->regs[rt.data] >> shift);
}

void op_div(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  int32_t reg_s = cpu->regs[rs.data];
  int32_t reg_t = cpu->regs[rt.data];

  if (reg_t == 0) {
    cpu->hi = reg_s;
    cpu->lo = (reg_s >= 0) ? 0xFFFFFFFF : 0x1;
  } else if (reg_s == 0x80000000 && reg_t == 0xFFFFFFFF) {
    cpu->hi = 0x0;
    cpu->lo = reg_s;
  } else {
    cpu->hi = reg_s % reg_t;
    cpu->lo = reg_s / reg_t;
  }
}

void op_divu(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  uint32_t reg_s = cpu->regs[rs.data];
  uint32_t reg_t = cpu->regs[rt.data];

  if (reg_t == 0) {
    cpu->hi = reg_s;
    cpu->lo = 0xFFFFFFFF;
  } else {
    cpu->hi = reg_s % reg_t;
    cpu->lo = reg_s / reg_t;
  }
}

void op_mflo(Cpu *cpu, Ins ins) {
  RegIndex rd = get_rd(ins);

  set_reg(cpu, rd, cpu->lo);
}

void op_mtlo(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);

  cpu->lo = cpu->regs[rs.data];
}

void op_mfhi(Cpu *cpu, Ins ins) {
  RegIndex rd = get_rd(ins);

  set_reg(cpu, rd, cpu->hi);
}

void op_mthi(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);

  cpu->hi = cpu->regs[rs.data];
}

void op_syscall(Cpu *cpu, Ins ins) {
  exception(cpu, SysCall);
}

void decode_and_execute(Cpu *cpu, Ins ins) {
  switch (get_func(ins)) {
    case 0xF:
      op_lui(cpu, ins);
      break;
    case 0xA:
      op_slti(cpu, ins);
      break;
    case 0xB:
      op_sltiu(cpu, ins);
      break;
    case 0xC:
      op_andi(cpu, ins);
      break;
    case 0xD:
      op_ori(cpu, ins);
      break;
    case 0x2B:
      op_sw(cpu, ins);
      break;
    case 0x0:
      switch (get_sub_func(ins)) {
        case 0x0:
          op_sll(cpu, ins);
          break;
        case 0x2:
          op_srl(cpu, ins);
          break;
        case 0x3:
          op_sra(cpu, ins);
          break;
        case 0x9:
          op_jalr(cpu, ins);
          break;
        case 0xC:
          op_syscall(cpu, ins);
          break;
        case 0x10:
          op_mfhi(cpu, ins);
          break;
        case 0x11:
          op_mthi(cpu, ins);
          break;
        case 0x12:
          op_mflo(cpu, ins);
          break;
        case 0x13:
          op_mtlo(cpu, ins);
          break;
        case 0x1A:
          op_div(cpu, ins);
          break;
        case 0x1B:
          op_divu(cpu, ins);
          break;
        case 0x24:
          op_and(cpu, ins);
          break;
        case 0x25:
          op_or(cpu, ins);
          break;
        case 0x2A:
          op_slt(cpu, ins);
          break;
        case 0x2B:
          op_sltu(cpu, ins);
          break;
        case 0x20:
          op_add(cpu, ins);
          break;
        case 0x21:
          op_addu(cpu, ins);
          break;
        case 0x23:
          op_subu(cpu, ins);
          break;
        case 0x8:
          op_jr(cpu, ins);
          break;
        default:
          log_ins(ins);
          fatal("DecodeError: Unhandled instruction: 0x%08X", ins);
      }
      break;
    case 0x9:
      op_addiu(cpu, ins);
      break;
    case 0x2:
      op_j(cpu, ins);
      break;
    case 0x10:
      switch (get_cop_func(ins)) {
        case 0x0:
          op_mfc0(cpu, ins);
          break;
        case 0x4:
          op_mtc0(cpu, ins);
          break;
        case 0x10:
          op_rfe(cpu, ins);
          break;
        default:
          log_ins(ins);
          fatal("DecodeError: Unhandled instruction: 0x%08X", ins);
      }
      break;
    case 0x1:
      op_bxx(cpu, ins);
      break;
    case 0x4:
      op_beq(cpu, ins);
      break;
    case 0x5:
      op_bne(cpu, ins);
      break;
    case 0x6:
      op_blez(cpu, ins);
      break;
    case 0x7:
      op_bgtz(cpu, ins);
      break;
    case 0x8:
      op_addi(cpu, ins);
      break;
    case 0x20:
      op_lb(cpu, ins);
      break;
    case 0x23:
      op_lw(cpu, ins);
      break;
    case 0x24:
      op_lbu(cpu, ins);
      break;
    case 0x29:
      op_sh(cpu, ins);
      break;
    case 0x28:
      op_sb(cpu, ins);
      break;
    case 0x3:
      op_jal(cpu, ins);
      break;
    default:
      log_ins(ins);
      fatal("DecodeError: Unhandled instruction: 0x%08X", ins);
  }
}

void destroy_cpu(Cpu *cpu) {
  destroy_interconnect(&cpu->inter);
}
