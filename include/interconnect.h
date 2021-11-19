#include "bios.h"
#include "ram.h"
#include "instruction.h"

#ifndef INTERCONNECT_H
#define INTERCONNECT_H

typedef struct Interconnect {
  Bios bios;
  Ram ram;
} Interconnect;

Interconnect init_interconnect(char const *bios_filename);
uint32_t load_inter32(Interconnect *inter, Addr addr);
uint8_t load_inter8(Interconnect *inter, Addr addr);
void store_inter32(Interconnect *inter, Addr addr, uint32_t val);
void store_inter16(Interconnect *inter, Addr addr, uint16_t val);
void store_inter8(Interconnect *inter, Addr addr, uint8_t val);
void destroy_interconnect(Interconnect *inter);

#endif
