#include <stdint.h>

#include "range.h"

#ifndef BIOS_H
#define BIOS_H

typedef struct Bios {
  uint8_t data[BIOS_SIZE];
} Bios;

Bios init_bios(char const *filename);
uint32_t load_bios32(Bios *bios, Addr offset);

#endif
