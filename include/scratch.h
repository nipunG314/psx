#include <stdint.h>

#include "range.h"

#ifndef SCRATCHPAD_H
#define SCRATCHPAD_H

typedef struct Scratchpad {
  uint8_t *data;
} Scratchpad;

Scratchpad init_scratchpad();
uint32_t load_scratchpad32(Scratchpad *scratchpad, Addr offset);
uint16_t load_scratchpad16(Scratchpad *scratchpad, Addr offset);
uint8_t load_scratchpad8(Scratchpad *Ram, Addr offset);
void store_scratchpad32(Scratchpad *scratchpad, Addr offset, uint32_t val);
void store_scratchpad16(Scratchpad *scratchpad, Addr offset, uint16_t val);
void store_scratchpad8(Scratchpad *scratchpad, Addr offset, uint8_t val);
void destroy_scratchpad(Scratchpad *scratchpad);

#endif
