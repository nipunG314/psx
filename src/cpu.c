#include <stdlib.h>

#include "cpu.h"
#include "log.h"
#include "interconnect.h"
#include "instruction.h"

Cpu init_cpu(char const *bios_filename) {
  Cpu cpu = {0};
  
  cpu.pc = BIOS_START;
  for(int index = 0; index < 32; index++)
    cpu.regs[index] = 0xDEADDEAD;
  cpu.regs[0] = 0x0;
  cpu.next_ins = 0x0;
  cpu.inter = init_interconnect(bios_filename);

  return cpu;
}

uint32_t load32(Cpu *cpu, uint32_t addr) {
  return load_inter32(&cpu->inter, addr);
}

void store32(Cpu *cpu, uint32_t addr, uint32_t val) {
  store_inter32(&cpu->inter, addr, val);
}

void set_reg(Cpu *cpu, uint8_t index, uint32_t value) {
  cpu->regs[index] = value;
  cpu->regs[0] = 0x0;
}

void run_next_ins(Cpu *cpu) {
  uint32_t ins = cpu->next_ins;

  cpu->next_ins = load32(cpu, cpu->pc);

  cpu->pc += 4;

  decode_and_execute(cpu, ins);
}

void op_lui(Cpu *cpu, uint32_t ins) {
  uint32_t imm = get_imm(ins);
  uint32_t rt = get_rt(ins);

  set_reg(cpu, rt, imm << 16);
}

void op_ori(Cpu *cpu, uint32_t ins) {
  uint32_t imm = get_imm(ins);
  uint32_t rs = get_rs(ins);
  uint32_t rt = get_rt(ins);

  set_reg(cpu, rt, imm | cpu->regs[rs]);
}

void op_sw(Cpu *cpu, uint32_t ins) {
  uint32_t imm_se = get_imm_se(ins);
  uint32_t rs = get_rs(ins);
  uint32_t rt = get_rt(ins);

  store32(cpu, imm_se + cpu->regs[rs], cpu->regs[rt]);
}

void op_sll(Cpu *cpu, uint32_t ins) {
  uint32_t shift = get_shift(ins);
  uint32_t rt = get_rt(ins);
  uint32_t rd = get_rd(ins);

  set_reg(cpu, rd, cpu->regs[rt] << shift);
}

void op_addiu(Cpu *cpu, uint32_t ins) {
  uint32_t imm_se = get_imm_se(ins);
  uint32_t rs = get_rs(ins);
  uint32_t rt = get_rt(ins);

  set_reg(cpu, rt, cpu->regs[rs] + imm_se);
}

void op_j(Cpu *cpu, uint32_t ins) {
  uint32_t imm_jump = get_imm_jump(ins);

  cpu->pc = (cpu->pc & 0xF0000000) | (imm_jump << 2);
}

void op_or(Cpu *cpu, uint32_t ins) {
  uint32_t rs = get_rs(ins);
  uint32_t rt = get_rt(ins);
  uint32_t rd = get_rd(ins);

  set_reg(cpu, rd, cpu->regs[rs] | cpu->regs[rt]);
}

void log_ins(uint32_t ins) {
  uint32_t func = get_func(ins);
  log_trace("ins_func: 0x%X", func);
  if (func == 0x0) {
    log_trace("ins_sub_func: 0x%X", get_sub_func(ins));
  }
  fatal("DecodeError: Unhandled instruction: 0x%X", ins);
}

void decode_and_execute(Cpu *cpu, uint32_t ins) {
  switch (get_func(ins)) {
    case 0xF:
      op_lui(cpu, ins);
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
        case 0x25:
          op_or(cpu, ins);
          break;
        default:
          log_ins(ins);
      }
      break;
    case 0x9:
      op_addiu(cpu, ins);
      break;
    case 0x2:
      op_j(cpu, ins);
      break;
    default:
      log_ins(ins);
  }
}
