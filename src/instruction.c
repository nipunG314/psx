#include "instruction.h"

char const *funcs[] = {
  "", "BcondZ", "J", "JAL", "BEQ", "BNE", "BLEZ", "BGTZ",
  "ADDI", "ADDIU", "SLTI", "SLTIU", "ANDI", "ORI", "XORI", "LUI",
  "COP0", "COP1", "COP2", "COP3", "N/A", "N/A", "N/A", "N/A",
  "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
  "LB", "LH", "LWL", "LW", "LBU", "LHU", "LWR", "N/A",
  "SB", "SH", "SWL", "SW", "N/A", "N/A", "SWR", "N/A",
  "LWC0", "LWC1", "LWC2", "LWC3", "N/A", "N/A", "N/A", "N/A",
  "SWC0", "SWC1", "SWC2", "SWC3", "N/A", "N/A", "N/A", "N/A"
};

char const *special_funcs[] = {
  "SLL", "N/A", "SRL", "SRA", "SLLV", "N/A", "SRLV", "SRAV",
  "JR", "JALR", "N/A", "N/A", "SYSCALL", "BREAK", "N/A", "N/A",
  "MFHI", "MTHI", "MFLO", "MTLO", "N/A", "N/A", "N/A", "N/A",
  "MULT", "MULTU", "DIV", "DIVU", "N/A", "N/A", "N/A", "N/A",
  "ADD", "ADDU", "SUB", "SUBU", "AND", "OR", "XOR", "NOR",
  "N/A", "N/A", "SLT", "SLTU", "N/A", "N/A", "N/A", "N/A",
  "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
  "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A"
};

uint32_t get_func(Ins ins) {
  return ins.data >> 26;
}

uint32_t get_sub_func(Ins ins) {
  return ins.data & 0x3F;
}

uint32_t get_cop_func(Ins ins) {
  return (ins.data >> 21) & 0x1F;
}

RegIndex get_rs(Ins ins) {
  return MAKE_RegIndex((ins.data >> 21) & 0x1F);
}

RegIndex get_rt(Ins ins) {
  return MAKE_RegIndex((ins.data >> 16) & 0x1F);
}

RegIndex get_rd(Ins ins) {
  return MAKE_RegIndex((ins.data >> 11) & 0x1F);
}

uint32_t get_shift(Ins ins) {
  return (ins.data >> 6) & 0x1F;
}

uint32_t get_imm(Ins ins) {
  return ins.data & 0xFFFF;
}

uint32_t get_imm_se(Ins ins) {
  int16_t tmp = ins.data & 0xFFFF;

  return tmp;
}

uint32_t get_imm_jump(Ins ins) {
  return ins.data & 0x3FFFFFF;
}

uint8_t get_cop_reg(Ins ins) {
  return get_rd(ins).data;
}
