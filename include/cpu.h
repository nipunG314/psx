#include <stdint.h>
#include <stdbool.h>

#include "types.h"

#ifndef CPU_H
#define CPU_H

typedef struct Psx Psx;

typedef void (*OpFunc)(Psx *, Ins);

typedef struct LoadAttribute {
  uint32_t val;
  uint8_t cycle_count;
} LoadAttribute;

static inline LoadAttribute MAKE_LoadAttribute(uint32_t val, uint8_t cycle_count) {
  LoadAttribute load_attribute;

  load_attribute.val = val;
  load_attribute.cycle_count = cycle_count;

  return load_attribute;
}

typedef struct Load {
  bool active;
  RegIndex index;
  uint32_t val;
  Cycles cycle_count;
} Load;

static inline Load MAKE_Load(bool active, RegIndex index, uint32_t val, Cycles cycle_count) {
  Load load;

  load.active = active;
  load.index = index;
  load.val = val;
  load.cycle_count = cycle_count;

  return load;
}

typedef struct Cpu {
  Addr pc;
  Addr next_pc;
  Addr current_pc;
  uint32_t regs[32];
  uint32_t hi;
  uint32_t lo;
  Load load;
  RegIndex load_cycles_index;
  uint8_t load_cycles[32];
  bool branch;
  bool delay_slot;
  Cycles mult_div_end;
  uint8_t optable_offset;
} Cpu;

Cpu init_cpu();
void run_next_ins(Psx *psx);
void handle_irq_changed(Psx *psx);
void rebase_counters(Cpu *cpu, Cycles count);

#endif
