#include "instruction.h"

RegIndex MAKE_RegIndex(uint8_t);

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
