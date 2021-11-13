#include <stdint.h>

#include "range.h"

#ifndef RAM_H
#define RAM_H

typedef struct Ram {
  uint8_t *data;
} Ram;

Ram init_ram();
uint32_t load_ram32(Ram *ram, Addr offset);
void store_ram32(Ram *ram, Addr offset, uint32_t val);
void destroy_ram(Ram *ram);

#endif
