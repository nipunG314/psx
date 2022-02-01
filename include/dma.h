#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include <stdbool.h>

#include "types.h"

#define DMA_REFRESH_PERIOD 128

typedef enum DmaSync {
  Manual,
  Request,
  LinkedList
} DmaSync;

typedef enum DmaPort {
  DmaMdecIn,
  DmaMdecOut,
  DmaGpu,
  DmaCdRom,
  DmaSpu,
  DmaPio,
  DmaOtc,
  DmaChannelCount
} DmaPort;

typedef struct DmaIrq {
  uint8_t unknown_field;
  bool force_irq;
  uint8_t channel_irq_enable;
  bool master_irq_enable;
  uint8_t channel_irq_flag;
  bool master_irq_flag;
} DmaIrq;

DmaIrq init_dma_irq();

uint32_t get_dma_irq(DmaIrq *irq);
bool set_dma_irq(DmaIrq *irq, uint32_t val);
bool refresh_dma_irq(DmaIrq *irq);

static inline bool flag_dma_irq(DmaIrq *irq, DmaPort port) {
  irq->channel_irq_flag |= 1 << port;
  return refresh_dma_irq(irq);
}

static inline bool get_dma_irq_enable(DmaIrq *irq, DmaPort port) {
  return irq->channel_irq_enable & (1 << port);
}

typedef struct Psx Psx;

typedef struct DmaChannelControl {
  bool transfer_from_ram;
  bool transfer_backwards;
  bool chopping;
  DmaSync sync_mode;
  uint8_t chopping_dma_size;
  uint8_t chopping_cpu_size;
  bool enable;
  bool start;
} DmaChannelControl;

DmaChannelControl init_dma_channel_control(uint32_t val);
uint32_t get_dma_channel_control(DmaChannelControl *control);
void set_dma_channel_control(Psx *psx, DmaPort port, uint32_t val);

typedef struct DmaChannel {
  DmaChannelControl control;
  Addr base_addr;
  Addr cur_addr;
  uint16_t block_size;
  uint16_t block_count;
  uint16_t remaining_words;
  Cycles clock_counter;
} DmaChannel;

DmaChannel init_dma_channel();
void run_dma_channel(Psx *psx, DmaPort port, Cycles elapsed);

static inline uint32_t get_dma_block_control(DmaChannel *channel) {
  uint32_t res = channel->block_count;
  res = (res << 16) | channel->block_size;

  return res;
}

static inline void set_dma_block_control(DmaChannel *channel, uint32_t val) {
  channel->block_size = val & 0xFFFF;
  channel->block_count = (val >> 16) & 0xFFFF;
}

typedef struct Dma {
  uint32_t control;
  DmaIrq irq;
  DmaChannel channels[DmaChannelCount];
  Cycles dma_period_counter;
} Dma;

Dma init_dma();

uint32_t load_dma(Dma *dma, Addr offset, AddrType type);
void store_dma(Psx *psx, Addr offset, uint32_t val, AddrType type);

bool dma_can_run(Psx *psx, DmaPort port);
void refresh_cpu_halt(Psx *psx);

static inline bool end_dma(Dma *dma, DmaPort port) {
  dma->channels[port].control.start = false;
  dma->channels[port].control.enable = false;

  if (get_dma_irq_enable(&dma->irq, port))
    return flag_dma_irq(&dma->irq, port);

  return false;
}

#endif
