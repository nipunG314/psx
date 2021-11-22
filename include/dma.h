#include <stdint.h>

#include "instruction.h"

#ifndef DMA_H
#define DMA_H

typedef enum DmaDirection {
  ToRam,
  FromRam
} DmaDirection;

typedef enum DmaStep {
  Increment,
  Decrement
} DmaStep;

typedef enum DmaSync {
  Manual,
  Request,
  LinkedList
} DmaSync;

typedef enum DmaPort {
  MdecIn,
  MdecOut,
  Gpu,
  CdRom,
  Spu,
  Pio,
  Otc,
  DmaChannelCount
} DmaPort;

typedef struct DmaChannel {
  uint8_t enable;
  DmaDirection direction;
  DmaStep step;
  DmaSync sync;
  uint8_t trigger;
  uint8_t chop;
  uint8_t chop_dma_size;
  uint8_t chop_cpu_size;
  uint8_t dummy;
  uint32_t base_address;
  uint16_t block_size;
  uint16_t block_count;
} DmaChannel;

DmaChannel init_dma_channel();
uint32_t get_dma_channel_control(DmaChannel *channel);
void set_dma_channel_control(DmaChannel *channel, uint32_t val);
static inline void set_dma_channel_base_address(DmaChannel *channel, uint32_t val) {
  channel->base_address = val & 0x00FFFFFF;
}
static inline uint32_t get_dma_channel_block_control(DmaChannel *channel) {
  uint32_t block_size = channel->block_size;
  uint32_t block_count = channel->block_count;

  return (block_count << 16) | block_size;
}
static inline void set_dma_channel_block_control(DmaChannel *channel, uint32_t val) {
  channel->block_size = val;
  channel->block_count = (val >> 16);
}

typedef struct Dma {
  uint32_t control;
  uint8_t irq_master_en;
  uint8_t irq_channel_en;
  uint8_t irq_channel_flags;
  uint8_t irq_force;
  uint8_t irq_dummy;
  DmaChannel channels[DmaChannelCount];
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
