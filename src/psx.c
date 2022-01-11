#include <stdlib.h>

#include "psx.h"
#include "log.h"

Psx init_psx(char const *bios_file_path) {
  Psx psx;

  psx.cycles_counter = MAKE_Cycles(0);
  psx.dma_timing_penalty = MAKE_Cycles(0);
  psx.bios = init_bios(bios_file_path); 
  psx.ram = init_ram();

  return psx;
}

uint32_t load(Psx *psx, Addr addr, AddrType type) {
  addr = mask_region(addr);

  tick(psx, psx->dma_timing_penalty);
  psx->dma_timing_penalty.data = 0;

  int32_t offset = range_contains(range(BIOS), addr);
  if (offset >= 0) {
    return load_bios(&psx->bios, MAKE_Addr(offset), type);
  }

  offset = range_contains(range(RAM), addr);
  if (offset >= 0) {
    tick(psx, MAKE_Cycles(3));
    return load_ram(&psx->ram, MAKE_Addr(offset), type);
  }

  offset = range_contains(range(SPU), addr);
  if (offset >= 0) {
    tick(psx, MAKE_Cycles(type == AddrWord ? 36 : 16));
    log_error("Unhandled read from SPU. addr: 0x%08X, type: %d", addr, type);
    return 0;
  }

  offset = range_contains(range(DMA), addr);
  if (offset >= 0) {
    tick(psx, MAKE_Cycles(1));
    log_error("Unhandled read from DMA. addr: 0x%08X, type: %d", addr, type);
    return 0;
  }

  offset = range_contains(range(TIMERS), addr);
  if (offset >= 0) {
    tick(psx, MAKE_Cycles(1));
    log_error("Unhandled read from TIMERS. addr: 0x%08X, type: %d", addr, type);
    return 0;
  }

  offset = range_contains(range(GPU), addr);
  if (offset >= 0) {
    tick(psx, MAKE_Cycles(1));
    log_error("Unhandled read from GPU. addr: 0x%08X, type: %d", addr, type);
    return 0;
  }

  offset = range_contains(range(PAD_MEMCARD), addr);
  if (offset >= 0) {
    tick(psx, MAKE_Cycles(1));
    log_error("Unhandled read from PAD_MEMCARD. addr: 0x%08X, type: %d", addr, type);
    return 0;
  }

  offset = range_contains(range(EXPANSION1), addr);
  if (offset >= 0)
    return !0;

  fatal("Unhandled load call. Address: 0x%08X, Type: %d", addr, type);
}

void store(Psx *psx, Addr addr, uint32_t val, AddrType type) {
  addr = mask_region(addr);

  int32_t offset = range_contains(range(RAM), addr);
  if (offset >= 0) {
    store_ram(&psx->ram, MAKE_Addr(offset), val, type);
    return;
  }

  offset = range_contains(range(SCRATCH_PAD), addr);
  if (offset >= 0) {
    log_error("Unhandled write to SCRATCH_PAD. addr: 0x%08X, val: 0x%08X, type: %d", addr, val, type);
    return;
  }

  offset = range_contains(range(SPU), addr);
  if (offset >= 0) {
    log_error("Unhandled write to SCRATCH_PAD. addr: 0x%08X, val: 0x%08X, type: %d", addr, val, type);
    return;
  }

  offset = range_contains(range(DMA), addr);
  if (offset >= 0) {
    log_error("Unhandled write to SCRATCH_PAD. addr: 0x%08X, val: 0x%08X, type: %d", addr, val, type);
    return;
  }

  offset = range_contains(range(TIMERS), addr);
  if (offset >= 0) {
    log_error("Unhandled write to SCRATCH_PAD. addr: 0x%08X, val: 0x%08X, type: %d", addr, val, type);
    return;
  }

  offset = range_contains(range(GPU), addr);
  if (offset >= 0) {
    log_error("Unhandled write to SCRATCH_PAD. addr: 0x%08X, val: 0x%08X, type: %d", addr, val, type);
    return;
  }

  fatal("Unhandled write call. Address: 0x%08X, Type: %d", addr, type);
}
