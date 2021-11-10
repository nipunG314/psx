#include <stdint.h>

#ifndef BIOS_H
#define BIOS_H

#define BIOS_SIZE 512 * 1024

typedef struct Bios {
  uint8_t data[BIOS_SIZE];
} Bios;

Bios init_bios(char const *filename);
uint32_t load_ins(Bios *bios, uint32_t offset);

#endif
