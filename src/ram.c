#include <stdlib.h>

#include "ram.h"

Ram init_ram() {
  Ram ram;
  ram.data = calloc(range(RAM).size, sizeof(uint8_t));

  for(int index = 0; index < range(RAM).size; index++)
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

uint16_t load_ram16(Ram *ram, Addr offset) {

  uint16_t b0 = ram->data[offset.data++];
  uint16_t b1 = ram->data[offset.data++];

  return b0 | (b1 << 8);
}

uint8_t load_ram8(Ram *ram, Addr offset) {
  return ram->data[offset.data];
}

uint32_t load_ram(Ram *ram, Addr offset, AddrType type) {
  uint32_t res = 0;
  uint32_t res_mask = -1;
  res_mask = res_mask >> (32 - type * 8);

  for(size_t byte_count = 0; byte_count < type; byte_count++) {
    uint32_t byte = ram->data[offset.data++];
    res |= byte << (byte_count * 8);
  }

  return res & res_mask;
}

void store_ram(Ram *ram, Addr offset, uint32_t val, AddrType type) {
  uint32_t val_mask = -1;
  val_mask = val_mask >> (32 - type * 8);
  val &= val_mask;

  for(size_t byte_count = 0; byte_count < type; byte_count++) {
    ram->data[offset.data++] = val >> (byte_count * 8);
  }
}

void destroy_ram(Ram *ram) {
  free(ram->data);
}
