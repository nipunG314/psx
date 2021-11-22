#include <stdlib.h>

#include "dma.h"
#include "log.h"

Dma init_dma() {
  Dma dma;
  dma.control = 0x07654321;

  return dma;
}

uint32_t get_dma_reg(Dma *dma, Addr offset) {
  switch (offset.data) {
    case 0x70:
      return dma->control;
    default:
      fatal("Unhandled DMA Read. addr: 0x%08X", offset);
  }
}

void set_dma_reg(Dma *dma, Addr offset, uint32_t val) {
  switch (offset.data) {
    case 0x70:
      dma->control = val;
      break;
    default:
      fatal("Unhandled DMA Write. addr: 0x%08X", offset);
  }
}
