#include <stdint.h>

#include "interconnect.h"

#ifndef CPU_H
#define CPU_H

typedef struct Cpu {
  uint32_t pc;
  uint32_t regs[32];
  uint32_t next_ins;
  Interconnect inter;
} Cpu;

Cpu init_cpu(char const *bios_filename);
uint32_t load32(Cpu *cpu, uint32_t addr);
void store32(Cpu *cpu, uint32_t addr, uint32_t val);
void set_reg(Cpu *cpu, uint8_t index, uint32_t value);
void decode_and_execute(Cpu *cpu, uint32_t ins);
void run_next_ins(Cpu *cpu);

#endif
