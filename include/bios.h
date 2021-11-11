#include <stdint.h>

#ifndef BIOS_H
#define BIOS_H

#define BIOS_START 0xBFC00000
#define BIOS_SIZE 512 * 1024

typedef struct Bios {
  uint8_t data[BIOS_SIZE];
} Bios;

Bios init_bios(char const *filename);
uint32_t load_bios32(Bios *bios, uint32_t offset);

#endif
