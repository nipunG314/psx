#include <stdint.h>

#include "types.h"
#include "bios.h"
#include "ram.h"
#include "sync.h"
#include "irq.h"
#include "cpu.h"
#include "cop0.h"
#include "timers.h"

#ifndef PSX_H
#define PSX_H

typedef struct RomHeader {
  Addr initial_pc;
  uint32_t initial_r28;
  Addr ram_address;
  Addr data_start;
  uint32_t data_size;
  Addr bss_start;
  uint32_t bss_size;
  Addr r29_start;
  uint32_t r29_size;
} RomHeader;

typedef struct Psx {
  Cycles cycles_counter;
  Cycles dma_timing_penalty;
  Bios bios; 
  Ram ram;
  Synchronizer sync;
  IrqState irq;
  Cpu cpu;
  Cop0 cop0;
  Timers timers;
} Psx;

Psx init_psx(char const *bios_file_path);

uint32_t load(Psx *psx, Addr addr, AddrType type);
void store(Psx *psx, Addr addr, uint32_t val, AddrType type);

static inline void tick(Psx *psx, Cycles cycle_count) {
  psx->cycles_counter.data += cycle_count.data;
}

#endif
