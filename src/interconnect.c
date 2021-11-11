#include <stdlib.h>

#include "interconnect.h"
#include "log.h"

Interconnect init_interconnect(char const *bios_filename) {
  Interconnect inter = {0};

  inter.bios = init_bios(bios_filename);

  return inter;
}

uint32_t load_inter32(Interconnect *inter, uint32_t addr) {
  if (addr >= BIOS_START && addr < BIOS_START + BIOS_SIZE) {
    return load_bios32(&inter->bios, addr - BIOS_START);
  }

  log_error("Unhandled fetch call. Address: 0x%X", addr);
  exit(EXIT_FAILURE);
}

void store_inter32(Interconnect *inter, uint32_t addr, uint32_t val) {
  log_error("STUB: store_inter32: addr: 0x%X, val: 0x%X", addr, val);
  exit(EXIT_FAILURE);
}
