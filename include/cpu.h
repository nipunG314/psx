#include <stdint.h>

#include "instruction.h"
#include "interconnect.h"

#ifndef CPU_H
#define CPU_H

typedef struct Cpu {
  Addr pc;
  uint32_t regs[32];
  uint32_t sr;
  Ins next_ins;
  Interconnect inter;
} Cpu;

Cpu init_cpu(char const *bios_filename);
uint32_t load32(Cpu *cpu, Addr addr);
void store32(Cpu *cpu, Addr addr, uint32_t val);
void set_reg(Cpu *cpu, RegIndex index, uint32_t value);
void decode_and_execute(Cpu *cpu, Ins ins);
void run_next_ins(Cpu *cpu);

#endif
