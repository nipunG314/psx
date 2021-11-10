#include <stdint.h>

#include "interconnect.h"

#ifndef CPU_H
#define CPU_H

typedef struct Cpu {
  uint32_t pc;
  Interconnect inter;
} Cpu;

Cpu init_cpu(char const *bios_filename);

#endif
