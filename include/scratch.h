#include <stdint.h>

#include "range.h"

#ifndef SCRATCHPAD_H
#define SCRATCHPAD_H

typedef struct Scratchpad {
  uint8_t *data;
} Scratchpad;

Scratchpad init_scratchpad();
uint32_t load_scratchpad(Scratchpad *scratchpad, Addr offset, AddrType type);
void store_scratchpad(Scratchpad *scratchpad, Addr offset, uint32_t val, AddrType type);
void destroy_scratchpad(Scratchpad *scratchpad);

#endif
