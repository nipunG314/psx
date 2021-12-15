#include <stdlib.h>

#include "ram.h"
#include "flag.h"

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
  switch (type) {
    case AddrByte:
      return load_ram8(ram, offset);
    case AddrHalf:
      return load_ram16(ram, offset);
    case AddrWord:
      return load_ram32(ram, offset);
  }
}

void store_ram32(Ram *ram, Addr offset, uint32_t val) {

  uint8_t b0 = val;
  uint8_t b1 = val >> 8;
  uint8_t b2 = val >> 16;
  uint8_t b3 = val >> 24;

  ram->data[offset.data++] = b0; 
  ram->data[offset.data++] = b1; 
  ram->data[offset.data++] = b2; 
  ram->data[offset.data++] = b3; 
}

void store_ram16(Ram *ram, Addr offset, uint16_t val) {

  uint8_t b0 = val;
  uint8_t b1 = val >> 8;

  ram->data[offset.data++] = b0;
  ram->data[offset.data++] = b1;
}

void store_ram8(Ram *ram, Addr offset, uint8_t val) {
  ram->data[offset.data] = val;
}

void store_ram(Ram *ram, Addr offset, uint32_t val, AddrType type) {
  switch (type) {
    case AddrByte:
      store_ram8(ram, offset, val);
      break;
    case AddrHalf:
      store_ram16(ram, offset, val);
      break;
    case AddrWord:
      store_ram32(ram, offset, val);
      break;
  }
}

void destroy_ram(Ram *ram) {
  free(ram->data);
}
