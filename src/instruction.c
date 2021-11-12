#include "instruction.h"

uint32_t get_func(uint32_t ins) {
  return ins >> 26;
}

uint32_t get_sub_func(uint32_t ins) {
  return ins & 0x3F;
}

uint32_t get_rs(uint32_t ins) {
  return (ins >> 21) & 0x1F;
}

uint32_t get_rt(uint32_t ins) {
  return (ins >> 16) & 0x1F;
}

uint32_t get_rd(uint32_t ins) {
  return (ins >> 11) & 0x1F;
}

uint32_t get_shift(uint32_t ins) {
  return (ins >> 6) & 0x1F;
}

uint32_t get_imm(uint32_t ins) {
  return ins & 0xFFFF;
}

uint32_t get_imm_se(uint32_t ins) {
  int16_t tmp = ins & 0xFFFF;

  return tmp;
}

uint32_t get_imm_jump(uint32_t ins) {
  return ins & 0x3FFFFFF;
}
