#include <stdint.h>

#include "interconnect.h"

#ifndef CPU_H
#define CPU_H

typedef struct Cpu {
  uint32_t pc;
  Interconnect inter;
} Cpu;

Cpu init_cpu(char const *bios_filename);
uint32_t load_ins(Cpu *cpu, uint32_t addr);
void decode_and_execute(Cpu *cpu, uint32_t ins);
void run_next_ins(Cpu *cpu);

#endif
