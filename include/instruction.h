#include <stdint.h>

#include "log.h"

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#define DECLARE_TYPE(T, N) typedef struct N {\
  T data;\
} N;\
static inline N MAKE_##N(T x) {\
  N val = {x};\
  return val;\
}

extern char const *funcs[];
extern char const *special_funcs[];

typedef enum AddrType {
  AddrByte,
  AddrHalf,
  AddrWord
} AddrType;

DECLARE_TYPE(uint32_t, Ins)
DECLARE_TYPE(uint8_t, RegIndex)
DECLARE_TYPE(uint32_t, Addr)

static uint32_t const RegionMask[] = {
  // KUSEG - 2GB
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
  // KSEG0 - 512MB
  0x7FFFFFFF,
  // KSEG1 - 512MB
  0x1FFFFFFF,
  // KSEG2 - 1GB
  0xFFFFFFFF, 0xFFFFFFFF
};

static inline Addr mask_region(Addr addr) {
  uint8_t index = addr.data >> 29;

  return MAKE_Addr(addr.data & RegionMask[index]);
}

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

static inline void log_ins(Ins ins) {
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

#endif
