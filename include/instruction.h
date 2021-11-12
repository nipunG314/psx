#include <stdint.h>

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

uint32_t get_func(uint32_t ins);
uint32_t get_sub_func(uint32_t ins);
uint32_t get_rs(uint32_t ins);
uint32_t get_rt(uint32_t ins);
uint32_t get_rd(uint32_t ins);
uint32_t get_shift(uint32_t ins);
uint32_t get_imm(uint32_t ins);
uint32_t get_imm_se(uint32_t ins);
uint32_t get_imm_jump(uint32_t ins);

#endif
