#include <stdint.h>

#include "instruction.h"

#ifndef DMA_H
#define DMA_H

typedef struct Dma {
  uint32_t control;
} Dma;

Dma init_dma();
uint32_t get_dma_reg(Dma *dma, Addr offset);
void set_dma_reg(Dma *dma, Addr offset, uint32_t val);

#endif
