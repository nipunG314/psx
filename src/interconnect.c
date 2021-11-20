#include <stdlib.h>

#include "instruction.h"
#include "interconnect.h"
#include "log.h"

uint32_t const RegionMask[] = {
  // KUSEG - 2GB
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
  // KSEG0 - 512MB
  0x7FFFFFFF,
  // KSEG1 - 512MB
  0x1FFFFFFF,
  // KSEG2 - 1GB
  0xFFFFFFFF, 0xFFFFFFFF
};

Addr mask_region(Addr addr) {
  uint8_t index = addr.data >> 29;

  return MAKE_Addr(addr.data & RegionMask[index]);
}

Interconnect init_interconnect(char const *bios_filename) {
  Interconnect inter = {0};

  inter.bios = init_bios(bios_filename);
  inter.ram = init_ram();

  return inter;
}

uint32_t load_inter32(Interconnect *inter, Addr addr) {
  addr = mask_region(addr);

  int32_t offset = range_contains(range(BIOS), addr);
  if (offset >= 0)
    return load_bios32(&inter->bios, MAKE_Addr(offset));

  offset = range_contains(range(RAM), addr);
  if (offset >= 0)
    return load_ram32(&inter->ram, MAKE_Addr(offset));

  offset = range_contains(range(IRQ), addr);
  if (offset >= 0) {
    log_error("Unhandled read from IRQ. addr: 0x%08X", addr);
    return 0;
  }

  offset = range_contains(range(DMA), addr);
  if (offset >= 0) {
    log_error("Unhandled read from DMA. addr: 0x%08X", addr);
    return 0;
  }

  fatal("Unhandled load32 call. Address: 0x%08X", addr);
}

uint16_t load_inter16(Interconnect *inter, Addr addr) {
  addr = mask_region(addr);

  int32_t offset = range_contains(range(RAM), addr);
  if (offset >= 0)
    return load_ram16(&inter->ram, MAKE_Addr(offset));

  offset = range_contains(range(SPU), addr);
  if (offset >= 0) {
    log_error("Unhandled read from SPU. addr: 0x%08X", addr);
    return 0;
  }

  fatal("Unhandled load16 call. Address: 0x%08X", addr);
}

uint8_t load_inter8(Interconnect *inter, Addr addr) {
  addr = mask_region(addr);

  int32_t offset = range_contains(range(BIOS), addr);
  if (offset >= 0)
    return load_bios8(&inter->bios, MAKE_Addr(offset));

  offset = range_contains(range(RAM), addr);
  if (offset >= 0)
    return load_ram8(&inter->ram, MAKE_Addr(offset));

  offset = range_contains(range(EXPANSION1), addr);
  if (offset >= 0)
    return 0xFF;

  fatal("Unhandled load8 call. Address: 0x%08X", addr);
}

void store_inter32(Interconnect *inter, Addr addr, uint32_t val) {
  addr = mask_region(addr);

  int32_t offset = range_contains(range(MEM_CONTROL), addr);
  if (offset >= 0) {
    switch (offset) {
      case 0:
        if (val != 0x1F000000)
          fatal("Bad Expansion 1: Base Address: 0x%08X", val);
        break;
      case 4:
        if (val != 0x1F802000)
          fatal("Bad Expansion 2: Base Address: 0x%08X", val);
        break;
      default:
        log_error("Unhandled Write to MEM_CONTROL register. addr: 0x%08X. val: 0x%08X", addr, val);
    }

    return;
  }

  offset = range_contains(range(RAM), addr);
  if (offset >= 0) {
    store_ram32(&inter->ram, MAKE_Addr(offset), val);
    return;
  }

  offset = range_contains(range(RAM_SIZE), addr);
  if (offset >= 0) {
    log_error("Unhandled Write to RAM_SIZE register. addr: 0x%08X. val: 0x%08X", addr, val);
    return;
  }

  offset = range_contains(range(CACHE_CONTROL), addr);
  if (offset >= 0) {
    log_error("Unhandled Write to CACHE_CONTROL register. addr: 0x%08X. val: 0x%08X", addr, val);
    return;
  }

  offset = range_contains(range(IRQ), addr);
  if (offset >= 0) {
    log_error("Unhandled Write to IRQ register. addr: 0x%08X. val: 0x%08X", addr, val);
    return;
  }

  offset = range_contains(range(DMA), addr);
  if (offset >= 0) {
    log_error("Unhandled Write from DMA. addr: 0x%08X", addr);
    return;
  }


  fatal("Unhandled store32 call. addr: 0x%08X, val: 0x%08X", addr, val);
}

void store_inter16(Interconnect *inter, Addr addr, uint16_t val) {
  addr = mask_region(addr);

  int32_t offset = range_contains(range(RAM), addr);
  if (offset >= 0) {
    store_ram16(&inter->ram, MAKE_Addr(offset), val);
    return;
  }

  offset = range_contains(range(SPU), addr);
  if (offset >= 0) {
    log_error("Unhandled Write to SPU registers. addr: 0x%08X. val: 0x%08X", addr, val);
    return;
  }

  offset = range_contains(range(TIMERS), addr);
  if (offset >= 0) {
    log_error("Unhandled Write to TIMER registers. addr: 0x%08X. val: 0x%08X", addr, val);
    return;
  }

  fatal("Unhandled store16 call. addr: 0x%08X, val: 0x%08X", addr, val);
}

void store_inter8(Interconnect *inter, Addr addr, uint8_t val) {
  addr = mask_region(addr);

  int32_t offset = range_contains(range(RAM), addr);
  if (offset >= 0) {
    store_ram8(&inter->ram, MAKE_Addr(offset), val);
    return;
  }

  offset = range_contains(range(EXPANSION2), addr);
  if (offset >= 0) {
    log_error("Unhandled Write to EXPANSION2 registers. addr: 0x%08X. val: 0x%08X", addr, val);
    return;
  }

  fatal("Unhandled store8 call. addr: 0x%08X, val: 0x%08X", addr, val);
}

void destroy_interconnect(Interconnect *inter) {
  destroy_bios(&inter->bios);
  destroy_ram(&inter->ram);
}
