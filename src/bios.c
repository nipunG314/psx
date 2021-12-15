#include <stdlib.h>

#include "instruction.h"
#include "log.h"
#include "bios.h"

Bios init_bios(char const *filename) {
  Bios bios;
  bios.data = calloc(range(BIOS).size, sizeof(uint8_t));

  FILE * const fp = fopen(filename, "rb");

  if (!fp)
    fatal("IOError: Could not load BIOS: %s", filename);

  fseek(fp, 0L, SEEK_END);
  uint64_t count = ftell(fp);
  rewind(fp);

  if (fread(bios.data, sizeof(uint8_t), range(BIOS).size, fp) != sizeof(uint8_t) * count)
    fatal("IOError: Invalid BIOS Size: %s", filename);

  fclose(fp);

  return bios;
}

uint32_t load_bios32(Bios *bios, Addr offset) {

  uint32_t b0 = bios->data[offset.data++]; 
  uint32_t b1 = bios->data[offset.data++]; 
  uint32_t b2 = bios->data[offset.data++]; 
  uint32_t b3 = bios->data[offset.data++]; 

  return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

uint16_t load_bios16(Bios *bios, Addr offset) {

  uint16_t b0 = bios->data[offset.data++];
  uint16_t b1 = bios->data[offset.data++];

  return b0 | (b1 << 8);
}

uint8_t load_bios8(Bios *bios, Addr offset) {
  return bios->data[offset.data];
}

uint32_t load_bios(Bios *bios, Addr offset, AddrType type) {
  switch (type) {
    case AddrByte:
      return load_bios8(bios, offset);
    case AddrHalf:
      return load_bios16(bios, offset);
    case AddrWord:
      return load_bios32(bios, offset);
  }
}

void destroy_bios(Bios *bios) {
  free(bios);
}
