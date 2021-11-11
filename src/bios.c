#include <stdio.h>
#include <stdlib.h>

#include "log.h"
#include "bios.h"

Bios init_bios(char const *filename) {
  FILE * const fp = fopen(filename, "rb");

  if (!fp) {
    log_error("IOError: Could not load BIOS: %s", filename);
    exit(EXIT_FAILURE);
  }

  fseek(fp, 0L, SEEK_END);
  uint64_t count = ftell(fp);
  rewind(fp);

  Bios bios = {0};

  if (fread(bios.data, sizeof(uint8_t), BIOS_SIZE, fp) != sizeof(uint8_t) * count) {
    log_error("IOError: Invalid BIOS Size: %s", filename);
    exit(EXIT_FAILURE);
  }

  return bios;
}

uint32_t load_bios32(Bios *bios, uint32_t offset) {

  uint32_t b0 = bios->data[offset++]; 
  uint32_t b1 = bios->data[offset++]; 
  uint32_t b2 = bios->data[offset++]; 
  uint32_t b3 = bios->data[offset++]; 

  return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}
