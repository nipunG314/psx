#include <stdlib.h>

#include "interconnect.h"
#include "log.h"

Interconnect init_interconnect(char const *bios_filename) {
  Interconnect inter = {0};

  inter.bios = init_bios(bios_filename);

  return inter;
}

uint32_t load_ins(Interconnect *inter, uint32_t addr) {
  if (addr >= BIOS_START && addr < BIOS_START + BIOS_SIZE) {
    return load_bios_ins(&inter->bios, addr - BIOS_START);
  }

  log_error("Unhandled fetch call. Address: %X", addr);
  exit(EXIT_FAILURE);
}
