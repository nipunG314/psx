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
  uint32_t output_regs[32];
  uint32_t sr;
  uint32_t cause;
  Addr epc;
  uint32_t hi;
  uint32_t lo;
  LoadDelaySlot load_delay_slot;
  Interconnect inter;
  bool branch;
  bool delay_slot;
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
void destroy_cpu(Cpu *cpu);

#endif
