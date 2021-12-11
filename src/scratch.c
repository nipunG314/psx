#include <stdlib.h>

#include "scratch.h"
#include "flag.h"

Scratchpad init_scratchpad() {
  Scratchpad scratchpad;
  scratchpad.data = calloc(range(RAM).size, sizeof(uint8_t));

  for(int index = 0; index < range(RAM).size; index++)
    scratchpad.data[index] = 0xca;

  return scratchpad;
}

uint32_t load_scratchpad32(Scratchpad *scratchpad, Addr offset) {

  uint32_t b0 = scratchpad->data[offset.data++]; 
  uint32_t b1 = scratchpad->data[offset.data++]; 
  uint32_t b2 = scratchpad->data[offset.data++]; 
  uint32_t b3 = scratchpad->data[offset.data++]; 

  uint32_t ret = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);

  return ret;
}

uint16_t load_scratchpad16(Scratchpad *scratchpad, Addr offset) {

  uint16_t b0 = scratchpad->data[offset.data++];
  uint16_t b1 = scratchpad->data[offset.data++];

  return b0 | (b1 << 8);
}

uint8_t load_scratchpad8(Scratchpad *scratchpad, Addr offset) {
  return scratchpad->data[offset.data];
}

void store_scratchpad32(Scratchpad *scratchpad, Addr offset, uint32_t val) {

  uint8_t b0 = val;
  uint8_t b1 = val >> 8;
  uint8_t b2 = val >> 16;
  uint8_t b3 = val >> 24;

  scratchpad->data[offset.data++] = b0; 
  scratchpad->data[offset.data++] = b1; 
  scratchpad->data[offset.data++] = b2; 
  scratchpad->data[offset.data++] = b3; 
}

void store_scratchpad16(Scratchpad *scratchpad, Addr offset, uint16_t val) {

  uint8_t b0 = val;
  uint8_t b1 = val >> 8;

  scratchpad->data[offset.data++] = b0;
  scratchpad->data[offset.data++] = b1;
}

void store_scratchpad8(Scratchpad *scratchpad, Addr offset, uint8_t val) {
  scratchpad->data[offset.data] = val;
}

void destroy_scratchpad(Scratchpad *scratchpad) {
  free(scratchpad->data);
}
