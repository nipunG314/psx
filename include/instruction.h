#include <stdint.h>

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

uint32_t get_func(uint32_t ins);
uint32_t get_rs(uint32_t ins);
uint32_t get_rt(uint32_t ins);
uint32_t get_rd(uint32_t ins);
uint32_t get_imm(uint32_t ins);

#endif
