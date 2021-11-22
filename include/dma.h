#include <stdint.h>

#include "instruction.h"

#ifndef DMA_H
#define DMA_H

typedef struct Dma {
  uint32_t control;
  uint8_t irq_master_en;
  uint8_t irq_channel_en;
  uint8_t irq_channel_flags;
  uint8_t irq_force;
  uint8_t irq_dummy;
} Dma;

Dma init_dma();
uint32_t get_dma_reg(Dma *dma, Addr offset);
void set_dma_reg(Dma *dma, Addr offset, uint32_t val);

static inline uint8_t irq_status(Dma* dma) {
  uint8_t channel_irq = dma->irq_channel_flags & dma->irq_channel_en;

  return (dma->irq_force || (dma->irq_master_en && channel_irq));
}
uint32_t get_dma_interrupt(Dma* dma);
void set_dma_interrupt(Dma* dma, uint32_t val);
#endif
