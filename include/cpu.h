#include <stdint.h>

#ifndef CPU_H
#define CPU_H

typedef struct Cpu {
  uint32_t pc;
} Cpu;

Cpu init_cpu();

#endif
