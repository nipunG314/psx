#include "bios.h"
#include "ram.h"
#include "dma.h"
#include "gpu.h"
#include "timer.h"
#include "scratch.h"
#include "shared.h"
#include "instruction.h"

#ifndef INTERCONNECT_H
#define INTERCONNECT_H

typedef struct Interconnect {
  Bios bios;
  Ram ram;
  Dma dma;
  Gpu gpu;
  Timers timers;
  Scratchpad pad;
  size_t output_log_index;
} Interconnect;

Interconnect init_interconnect(char const *bios_filename);
uint32_t load(Interconnect *inter, SharedState *shared, Addr addr, AddrType type);
void store(Interconnect *inter, SharedState *shared, Addr addr, uint32_t val, AddrType type);
uint32_t get_dma_reg(Interconnect *inter, Addr offset);
void set_dma_reg(Interconnect *inter, Addr offset, uint32_t val);
void perform_dma(Interconnect *inter, DmaPort port);
void interconnect_sync(Interconnect *inter, SharedState *shared);

void destroy_interconnect(Interconnect *inter);

#endif
