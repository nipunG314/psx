#include <stdint.h>

#include "range.h"

#ifndef BIOS_H
#define BIOS_H

typedef struct Bios {
  uint8_t *data;
} Bios;

Bios init_bios(char const *filename);

uint32_t load_bios(Bios *bios, Addr offset, AddrType type);

void destroy_bios(Bios *bios);

#endif
