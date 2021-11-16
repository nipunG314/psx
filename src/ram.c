#include <stdlib.h>

#include "ram.h"

Ram init_ram() {
  Ram ram;
  ram.data = calloc(range(RAM).size, sizeof(uint8_t));

  for(int index = 0; index < RAM_SIZE; index++)
    ram.data[index] = 0xca;

  return ram;
}

uint32_t load_ram32(Ram *ram, Addr offset) {

  uint32_t b0 = ram->data[offset.data++]; 
  uint32_t b1 = ram->data[offset.data++]; 
  uint32_t b2 = ram->data[offset.data++]; 
  uint32_t b3 = ram->data[offset.data++]; 

  return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

void store_ram32(Ram *ram, Addr offset, uint32_t val) {

  uint8_t b0 = val;
  uint8_t b1 = val << 8;
  uint8_t b2 = val << 16;
  uint8_t b3 = val << 24;

  ram->data[offset.data++] = b0; 
  ram->data[offset.data++] = b1; 
  ram->data[offset.data++] = b2; 
  ram->data[offset.data++] = b3; 
}

void destroy_ram(Ram *ram) {
  free(ram->data);
}