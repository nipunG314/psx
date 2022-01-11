#include <stdint.h>

#include "types.h"
#include "bios.h"
#include "ram.h"

#ifndef PSX_H
#define PSX_H
typedef struct Psx {
  Cycles cycles_counter;
  Cycles dma_timing_penalty;
  Bios bios; 
  Ram ram;
} Psx;

Psx init_psx(char const *bios_file_path);

uint32_t load(Psx *psx, Addr addr, AddrType type);
void store(Psx *psx, Addr addr, uint32_t val, AddrType type);

static inline void tick(Psx *psx, Cycles cycle_count) {
  psx->cycles_counter.data += cycle_count.data;
}

#endif
