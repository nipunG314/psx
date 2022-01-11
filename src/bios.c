#include <stdlib.h>
#include <stdio.h>

#include "bios.h"
#include "log.h"

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

uint32_t load_bios(Bios *bios, Addr offset, AddrType type) {
  uint32_t res = 0;
  uint32_t res_mask = -1;
  res_mask = res_mask >> (32 - type * 8);

  for(size_t byte_count = 0; byte_count < type; byte_count++) {
    uint32_t byte = bios->data[offset.data++];
    res |= byte << (byte_count * 8);
  }

  return res & res_mask;
}

void destroy_bios(Bios *bios) {
  free(bios);
}
