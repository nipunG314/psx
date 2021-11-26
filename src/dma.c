#include <stdlib.h>

#include "dma.h"
#include "log.h"

DmaChannel init_dma_channel() {
  DmaChannel channel;

  channel.enable = false;
  channel.direction = DmaToRam;
  channel.step = DmaIncrement;
  channel.sync = DmaManual;
  channel.trigger = false;
  channel.chop = false;
  channel.chop_dma_size = 0;
  channel.chop_cpu_size = 0;
  channel.dummy = 0;
  channel.base_address = MAKE_Addr(0);
  channel.block_size = 0;
  channel.block_count = 0;
  channel.port = 0;

  return channel;
}

uint32_t get_dma_channel_control(DmaChannel *channel) {
  uint32_t control = 0;

  control |= ((uint32_t)channel->direction) << 0;
  control |= ((uint32_t)channel->step) << 1;
  control |= ((uint32_t)channel->chop) << 8;
  control |= ((uint32_t)channel->sync) << 9;
  control |= ((uint32_t)channel->chop_dma_size) << 16;
  control |= ((uint32_t)channel->chop_cpu_size) << 20;
  control |= ((uint32_t)channel->enable) << 24;
  control |= ((uint32_t)channel->trigger) << 28;
  control |= ((uint32_t)channel->dummy) << 29;

  return control;
}

void set_dma_channel_control(DmaChannel *channel, uint32_t val) {
  channel->direction = (val & 1) ? DmaFromRam : DmaToRam;
  channel->step = ((val >> 1) & 1) ? DmaDecrement: DmaIncrement;
  channel->chop = (val >> 8) & 1;
  switch ((val >> 9) & 3) {
    case 0:
      channel->sync = DmaManual;
      break;
    case 1:
      channel->sync = DmaRequest;
      break;
    case 2:
      channel->sync = DmaLinkedList;
      break;
    default:
      fatal("Unknown DMA Sync Mode: %d", (val >> 9) & 1);
  }
  channel->chop_dma_size = ((val >> 16) & 7);
  channel->chop_cpu_size = ((val >> 20) & 7);
  channel->enable = (val >> 24) & 1;
  channel->trigger = (val >> 28) & 1;
  channel->dummy = (val >> 29) & 3;
}

uint32_t get_dma_channel_transfer_size(DmaChannel *channel) {
  switch (channel->sync) {
    case DmaManual:
      return channel->block_size;
    case DmaRequest:
      {
        uint32_t block_size = channel->block_size;
        uint32_t block_count = channel->block_count;
        return block_size * block_count;
      }
    case DmaLinkedList:
      return 0;
  }

  return 0;
}

Dma init_dma() {
  Dma dma;
  dma.control = 0x07654321;
  for(size_t index = 0; index < DmaChannelCount; index++)
    dma.channels[index].port = index;

  return dma;
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

