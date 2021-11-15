#include <stdint.h>

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#define DECLARE_TYPE(T, N) typedef struct N {\
  T data;\
} N;\
static inline N MAKE_##N(T x) {\
  N val = {x};\
  return val;\
}

DECLARE_TYPE(uint32_t, Ins)
DECLARE_TYPE(uint8_t, RegIndex)
DECLARE_TYPE(uint32_t, Addr)

typedef struct LoadDelaySlot {
  RegIndex index;
  uint32_t val;
} LoadDelaySlot;

static inline LoadDelaySlot MAKE_LoadDelaySlot(RegIndex index, uint32_t val) {
  LoadDelaySlot tmp = {
    .index = index,
    .val = val,
  };

  return tmp;
}

uint32_t get_func(Ins ins);
uint32_t get_sub_func(Ins ins);
uint32_t get_cop_func(Ins ins);
RegIndex get_rs(Ins ins);
RegIndex get_rt(Ins ins);
RegIndex get_rd(Ins ins);
uint32_t get_shift(Ins ins);
uint32_t get_imm(Ins ins);
uint32_t get_imm_se(Ins ins);
uint32_t get_imm_jump(Ins ins);
uint8_t get_cop_reg(Ins ins);

#endif
