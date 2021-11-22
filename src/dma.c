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
    case 0x74:
      return get_dma_interrupt(dma);
    default:
      fatal("Unhandled DMA Read. addr: 0x%08X", offset);
  }
}

void set_dma_reg(Dma *dma, Addr offset, uint32_t val) {
  switch (offset.data) {
    case 0x70:
      dma->control = val;
      break;
    case 0x74:
      set_dma_interrupt(dma, val);
    default:
      fatal("Unhandled DMA Write. addr: 0x%08X", offset);
  }
}

uint32_t get_dma_interrupt(Dma* dma) {
  uint32_t interrupt_reg = 0x0;

  uint32_t dummy = dma->irq_dummy;
  uint32_t force = dma->irq_force;
  uint32_t channel_en = dma->irq_channel_en;
  uint32_t master = dma->irq_master_en;
  uint32_t channel_flags = dma->irq_channel_flags;
  uint32_t status = irq_status(dma);

  interrupt_reg |= dummy;
  interrupt_reg |= (force << 15);
  interrupt_reg |= (channel_en << 16);
  interrupt_reg |= (master << 23);
  interrupt_reg |= (channel_flags << 24);
  interrupt_reg |= (status << 31);

  return interrupt_reg;
}

void set_dma_interrupt(Dma* dma, uint32_t val) {
  dma->irq_dummy = (val & 0x3F);
  dma->irq_force = ((val >> 15) & 1);
  dma->irq_channel_en = ((val >> 16) & 0x7F);
  dma->irq_master_en = ((val >> 23) & 1);

  uint8_t ack = ((val >> 24) & 0x3F);
  dma->irq_channel_flags &= ~ack;
}
