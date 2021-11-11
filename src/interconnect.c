#include <stdlib.h>

#include "interconnect.h"
#include "log.h"

int32_t range_contains(uint32_t, uint32_t, uint32_t);

Interconnect init_interconnect(char const *bios_filename) {
  Interconnect inter = {0};

  inter.bios = init_bios(bios_filename);

  return inter;
}

uint32_t load_inter32(Interconnect *inter, uint32_t addr) {
  if (addr % 4)
    error("Unaligned load_inter32 addr: 0x%X", addr);

  int32_t offset = range_contains(BIOS_START, BIOS_SIZE, addr);
  if (offset >= 0)
    return load_bios32(&inter->bios, offset);

  error("Unhandled fetch call. Address: 0x%X", addr);
}

void store_inter32(Interconnect *inter, uint32_t addr, uint32_t val) {
  if (addr % 4)
    error("Unaligned store_inter32 addr: 0x%X", addr);

  error("STUB: store_inter32: addr: 0x%X, val: 0x%X", addr, val);
}
