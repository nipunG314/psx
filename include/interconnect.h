#include "bios.h"

#ifndef INTERCONNECT_H
#define INTERCONNECT_H

typedef struct Interconnect {
  Bios bios;
} Interconnect;

Interconnect init_interconnect(char const *bios_filename);
uint32_t load_ins(Interconnect *inter, uint32_t addr);

#endif
