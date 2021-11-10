#include "instruction.h"

uint32_t get_func(uint32_t ins) {
  return ins >> 26;
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

uint32_t get_imm(uint32_t ins) {
  return ins & 0xFFFF;
}
