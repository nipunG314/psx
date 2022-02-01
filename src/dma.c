#include "dma.h"
#include "gpu.h"
#include "psx.h"

Dma init_dma() {
  Dma dma;

  dma.control = 0;
  dma.irq = init_dma_irq();
  dma.dma_period_counter = MAKE_Cycles(0);
  for(DmaPort port = 0; port < DmaChannelCount; port++)
    dma.channels[port] = init_dma_channel();

  return dma;
}

bool dma_can_run(Psx *psx, DmaPort port) {
  bool write = psx->dma.channels[port].control.transfer_from_ram; 

  if (write) {
    switch (port) {
      case DmaGpu:
        return gpu_dma_write(&psx->gpu);
      default:
        fatal("dma_can_run check on port not defined! Port: %d", port);
    }
  }

  switch (port) {
    case DmaGpu:
      return true;
    default:
      fatal("dma_can_run check on port not defined! Port: %d", port);
  }
}

void refresh_cpu_halt(Psx *psx) {
  bool halt = false;
  for(size_t port = 0; port < DmaChannelCount; port++) {
    DmaChannel *channel = psx->dma.channels + port;
    bool check_halt = true;
    check_halt &= channel->control.enable; 
    check_halt &= !channel->control.chopping;
    check_halt &= (channel->control.sync_mode == Manual);
    halt |= check_halt;
  }

  int32_t timing_penalty = 0;
  if (!halt) {
    // Running the CPU in chopped mode
    // is only implemented for the GPU properly
    DmaChannel *gpu_channel = psx->dma.channels + DmaGpu;
    bool cpu_stalled = true;
    cpu_stalled &= gpu_channel->control.enable;
    cpu_stalled &= !gpu_channel->control.chopping;
    cpu_stalled &= (gpu_channel->control.sync_mode == Request);
    cpu_stalled &= (gpu_channel->block_size > 0);
    cpu_stalled &= dma_can_run(psx, DmaGpu);
    if (cpu_stalled)
      timing_penalty = gpu_channel->block_size - 1;
  }

  psx->dma_timing_penalty = MAKE_Cycles(fmin(200, timing_penalty));
  psx->cpu_stalled = halt; 
}

DmaChannel init_dma_channel() {
  DmaChannel channel;

  channel.control = init_dma_channel_control(0);
  channel.base_addr = MAKE_Addr(0);
  channel.cur_addr = channel.base_addr;
  channel.block_size = channel.block_count = 0;
  channel.remaining_words = 0;
  channel.clock_counter = MAKE_Cycles(0);

  return channel;
}

void run_dma_channel(Psx *psx, DmaPort port, Cycles cycle_count) {
  
}

DmaChannelControl init_dma_channel_control(uint32_t val) {
  DmaChannelControl control;

  control.transfer_from_ram = val & (1 << 0);
  control.transfer_backwards = val & (1 << 1);
  control.chopping = val & (1 << 8);
  control.sync_mode = (val >> 9) & 3;
  control.chopping_dma_size = (val >> 16) & 7;
  control.chopping_cpu_size = (val >> 20) & 7;
  control.enable = val & (val << 24);
  control.start = val & (val << 28);

  return control;
}

uint32_t get_dma_channel_control(DmaChannelControl *control) {
  uint32_t res = 0;

  res |= control->transfer_from_ram;
  res |= (1 << 1) * control->transfer_backwards;
  res |= (1 << 8) * control->chopping;
  res |= (control->sync_mode & 3) << 9;
  res |= (control->chopping_dma_size & 7) << 16;
  res |= (control->chopping_cpu_size & 7) << 20;
  res |= (1 << 24) * control->enable;
  res |= (1 << 28) * control->start;

  return res;
}

void set_dma_channel_control(Psx *psx, DmaPort port, uint32_t val) {
  DmaChannel *channel = psx->dma.channels + port;
  DmaChannelControl control;
  if (port != DmaOtc)
    control = init_dma_channel_control(val);
  else {
    control = init_dma_channel_control(0);
    channel->control.transfer_backwards = true;
  }
  
  bool prev_enable = channel->control.enable;
  bool next_enable = control.enable; 

  if (prev_enable ^ next_enable) {
    if (next_enable) {
      channel->control = control;
      channel->clock_counter = MAKE_Cycles(0);
      channel->remaining_words = 0;
      // Run channel for a small amount to trigger
      // race condition needed by some games
      run_dma_channel(psx, port, MAKE_Cycles(64));
    } else {
      channel->control.enable = false;
      run_dma_channel(psx, port, MAKE_Cycles(128 * 16));
      channel->control = control;
    }
  } else
    channel->control = control;

  refresh_cpu_halt(psx);
}

DmaIrq init_dma_irq() {
  DmaIrq irq;

  irq.unknown_field = 0;
  irq.force_irq = false;
  irq.channel_irq_enable = 0;
  irq.master_irq_enable = false;
  irq.channel_irq_flag = 0;
  irq.master_irq_flag = false;

  return irq;
}

uint32_t get_dma_irq(DmaIrq *irq) {
  uint32_t res = 0;
  uint32_t enable = irq->channel_irq_enable & 0x7F;
  uint32_t flag = irq->channel_irq_flag & 0x7F;

  res |= irq->unknown_field & 0x3F;
  res |= (1 << 15) * irq->force_irq;
  res |= enable << 16;
  res |= (1 << 23) * irq->master_irq_enable;
  res |= flag << 24;
  res |= (1 << 31) * irq->master_irq_flag;

  return res;
}

bool set_dma_irq(DmaIrq *irq, uint32_t val) {
  irq->unknown_field = val & 0x3F;
  irq->force_irq = (val >> 15) & 1;
  irq->channel_irq_enable = (val >> 16) & 0x7F;
  irq->master_irq_enable = (val >> 23) & 1;
  irq->channel_irq_flag &= !((val >> 24) & 0x7F);

  return refresh_dma_irq(irq);
}

bool refresh_dma_irq(DmaIrq *irq) {
  bool irq_active = (irq->channel_irq_flag != 0) && irq->master_irq_enable;
  if (irq_active || irq->force_irq) {
    if (irq->master_irq_flag)
      return false;

    return irq->master_irq_flag = true;
  }

  return irq->master_irq_flag = false;
}
