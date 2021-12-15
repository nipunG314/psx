#include <stdint.h>

#include "range.h"

#ifndef RAM_H
#define RAM_H

typedef struct Ram {
  uint8_t *data;
} Ram;

Ram init_ram();
uint32_t load_ram(Ram *ram, Addr offset, AddrType type);
void store_ram(Ram *ram, Addr offset, uint32_t val, AddrType type);
void destroy_ram(Ram *ram);

#endif
