#include <stdlib.h>

#include "instruction.h"
#include "interconnect.h"
#include "log.h"

int32_t range_contains(uint32_t, uint32_t, Addr);
Addr MAKE_Addr(uint32_t);

Interconnect init_interconnect(char const *bios_filename) {
  Interconnect inter = {0};

  inter.bios = init_bios(bios_filename);

  return inter;
}

uint32_t load_inter32(Interconnect *inter, Addr addr) {
  if (addr.data % 4)
    fatal("Unaligned load_inter32 addr: 0x%X", addr);

  int32_t offset = range_contains(BIOS_START, BIOS_SIZE, addr);
  if (offset >= 0)
    return load_bios32(&inter->bios, MAKE_Addr(offset));

  fatal("Unhandled load call. Address: 0x%X", addr);
}

void store_inter32(Interconnect *inter, Addr addr, uint32_t val) {
  if (addr.data % 4)
    fatal("Unaligned store_inter32 addr: 0x%X", addr);

  int32_t offset = range_contains(MEM_CONTROL_START, MEM_CONTROL_SIZE, addr);
  if (offset >= 0) {
    switch (offset) {
      case 0:
        if (val != 0x1F000000)
          fatal("Bad Expansion 1: Base Address: 0x%X", val);
        break;
      case 4:
        if (val != 0x1F802000)
          fatal("Bad Expansion 2: Base Address: 0x%X", val);
        break;
      default:
        log_error("Unhandled Write to MEM_CONTROL register");
    }

    return;
  }

  offset = range_contains(RAM_SIZE_START, RAM_SIZE_SIZE, addr);
  if (offset >= 0)
    return;

  offset = range_contains(CACHE_CONTROL_START, CACHE_CONTROL_SIZE, addr);
  if (offset >= 0)
    return;

  fatal("Unhandled store call. addr: 0x%X, val: 0x%X", addr, val);
}
