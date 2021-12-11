#include <stdint.h>
#include <stdbool.h>

#include "instruction.h"
#include "interconnect.h"
#include "exception.h"

#ifndef CPU_H
#define CPU_H

typedef struct Cpu {
  Addr pc;
  Addr next_pc;
  Addr current_pc;
  uint32_t regs[32];
  Addr bad_v_adr;
  uint32_t sr;
  uint32_t cause;
  Addr epc;
  uint32_t hi;
  uint32_t lo;
  LoadDelaySlot load_delay_slot;
  Interconnect inter;
  bool branch;
  bool delay_slot;
  size_t output_log_index;
} Cpu;

Cpu init_cpu(char const *bios_filename);
uint32_t load32(Cpu *cpu, Addr addr);
uint16_t load16(Cpu *cpu, Addr addr);
uint8_t load8(Cpu *cpu, Addr addr);
void store32(Cpu *cpu, Addr addr, uint32_t val);
void store16(Cpu *cpu, Addr addr, uint16_t val);
void store8(Cpu *cpu, Addr addr, uint8_t val);
void set_reg(Cpu *cpu, RegIndex index, uint32_t value);
void decode_and_execute(Cpu *cpu, Ins ins);
void run_next_ins(Cpu *cpu);
void exception(Cpu *cpu, Exception exp);

static inline void delayed_load(Cpu *cpu) {
  set_reg(cpu, cpu->load_delay_slot.index, cpu->load_delay_slot.val);
  cpu->load_delay_slot = MAKE_LoadDelaySlot(MAKE_RegIndex(0x0), 0x0);
}

static inline void delayed_load_chain(Cpu *cpu, RegIndex index, uint32_t val) {
  if (cpu->load_delay_slot.index.data != index.data)
    set_reg(cpu, cpu->load_delay_slot.index, cpu->load_delay_slot.val);
  cpu->load_delay_slot = MAKE_LoadDelaySlot(index, val);
}

void destroy_cpu(Cpu *cpu);

#endif
