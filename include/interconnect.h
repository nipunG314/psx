#include "bios.h"
#include "instruction.h"

#ifndef INTERCONNECT_H
#define INTERCONNECT_H

typedef struct Interconnect {
  Bios bios;
} Interconnect;

Interconnect init_interconnect(char const *bios_filename);
uint32_t load_inter32(Interconnect *inter, Addr addr);
void store_inter32(Interconnect *inter, Addr addr, uint32_t val);

#endif
