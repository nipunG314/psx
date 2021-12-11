#include <stdlib.h>

#include "instruction.h"
#include "interconnect.h"
#include "log.h"
#include "output_logger.h"
#include "flag.h"

Interconnect init_interconnect(char const *bios_filename) {
  Interconnect inter = {0};

  inter.bios = init_bios(bios_filename);
  inter.ram = init_ram();
  inter.dma = init_dma();
  inter.gpu = init_gpu();
  inter.output_log_index = init_output_log();

  return inter;
}

uint32_t load_inter32(Interconnect *inter, Addr addr) {
  addr = mask_region(addr);

  int32_t offset = range_contains(range(BIOS), addr);
  if (offset >= 0)
    return load_bios32(&inter->bios, MAKE_Addr(offset));

  offset = range_contains(range(RAM), addr);
  if (offset >= 0)
    return load_ram32(&inter->ram, MAKE_Addr(offset));

  offset = range_contains(range(IRQ), addr);
  if (offset >= 0) {
    log_error("Unhandled read from IRQ. addr: 0x%08X", addr);
    return 0;
  }

  offset = range_contains(range(DMA), addr);
  if (offset >= 0) {
    return get_dma_reg(inter, MAKE_Addr(offset));
  }

  offset = range_contains(range(GPU), addr);
  if (offset >= 0) {
    switch (offset) {
      case 0:
        return gpu_read(&inter->gpu);
      case 4:
        return gpu_status(&inter->gpu);
    }
  }

  offset = range_contains(range(TIMERS), addr);
  if (offset >= 0) {
    log_error("Unhandled read from TIMERS. addr: 0x%08X", addr);
    return 0;
  }

  fatal("Unhandled load32 call. Address: 0x%08X", addr);
}

uint16_t load_inter16(Interconnect *inter, Addr addr) {
  addr = mask_region(addr);

  int32_t offset = range_contains(range(RAM), addr);
  if (offset >= 0)
    return load_ram16(&inter->ram, MAKE_Addr(offset));

  offset = range_contains(range(SPU), addr);
  if (offset >= 0) {
    log_error("Unhandled read from SPU. addr: 0x%08X", addr);
    return 0;
  }

  offset = range_contains(range(IRQ), addr);
  if (offset >= 0) {
    log_error("Unhandled read from IRQ. addr: 0x%08X", addr);
    return 0;
  }

  fatal("Unhandled load16 call. Address: 0x%08X", addr);
}

uint8_t load_inter8(Interconnect *inter, Addr addr) {
  addr = mask_region(addr);

  int32_t offset = range_contains(range(BIOS), addr);
  if (offset >= 0)
    return load_bios8(&inter->bios, MAKE_Addr(offset));

  offset = range_contains(range(RAM), addr);
  if (offset >= 0)
    return load_ram8(&inter->ram, MAKE_Addr(offset));

  offset = range_contains(range(EXPANSION1), addr);
  if (offset >= 0)
    return 0xFF;

  fatal("Unhandled load8 call. Address: 0x%08X", addr);
}

void store_inter32(Interconnect *inter, Addr addr, uint32_t val) {
  addr = mask_region(addr);

  int32_t offset = range_contains(range(MEM_CONTROL), addr);
  if (offset >= 0) {
    switch (offset) {
      case 0:
        if (val != 0x1F000000)
          fatal("Bad Expansion 1: Base Address: 0x%08X", val);
        break;
      case 4:
        if (val != 0x1F802000)
          fatal("Bad Expansion 2: Base Address: 0x%08X", val);
        break;
      default:
        log_error("Unhandled Write to MEM_CONTROL register. addr: 0x%08X. val: 0x%08X", addr, val);
    }

    return;
  }

  offset = range_contains(range(RAM), addr);
  if (offset >= 0) {
    store_ram32(&inter->ram, MAKE_Addr(offset), val);
    return;
  }

  offset = range_contains(range(RAM_SIZE), addr);
  if (offset >= 0) {
    log_error("Unhandled Write to RAM_SIZE register. addr: 0x%08X. val: 0x%08X", addr, val);
    return;
  }

  offset = range_contains(range(CACHE_CONTROL), addr);
  if (offset >= 0) {
    log_error("Unhandled Write to CACHE_CONTROL register. addr: 0x%08X. val: 0x%08X", addr, val);
    return;
  }

  offset = range_contains(range(IRQ), addr);
  if (offset >= 0) {
    log_error("Unhandled Write to IRQ register. addr: 0x%08X. val: 0x%08X", addr, val);
    return;
  }

  offset = range_contains(range(DMA), addr);
  if (offset >= 0) {
    set_dma_reg(inter, MAKE_Addr(offset), val);
    return;
  }

  offset = range_contains(range(GPU), addr);
  if (offset >= 0) {
    switch (offset) {
      case 0:
        gpu_gp0(&inter->gpu, val);
        break;
      case 4:
        gpu_gp1(&inter->gpu, val);
        break;
    }
    return;
  }

  offset = range_contains(range(TIMERS), addr);
  if (offset >= 0) {
    log_error("Unhandled Write to TIMERS register. addr: 0x%08X. val: 0x%08X", addr, val);
    return;
  }

  fatal("Unhandled store32 call. addr: 0x%08X, val: 0x%08X", addr, val);
}

void store_inter16(Interconnect *inter, Addr addr, uint16_t val) {
  addr = mask_region(addr);

  int32_t offset = range_contains(range(RAM), addr);
  if (offset >= 0) {
    store_ram16(&inter->ram, MAKE_Addr(offset), val);
    return;
  }

  offset = range_contains(range(SPU), addr);
  if (offset >= 0) {
    log_error("Unhandled Write to SPU registers. addr: 0x%08X. val: 0x%08X", addr, val);
    return;
  }

  offset = range_contains(range(TIMERS), addr);
  if (offset >= 0) {
    log_error("Unhandled Write to TIMER registers. addr: 0x%08X. val: 0x%08X", addr, val);
    return;
  }

  offset = range_contains(range(IRQ), addr);
  if (offset >= 0) {
    log_error("Unhandled Write from IRQ. addr: 0x%08X", addr);
    return;
  }

  LOG_PC();
  LOG_INS();
  fatal("Unhandled store16 call. addr: 0x%08X, val: 0x%08X", addr, val);
}

void store_inter8(Interconnect *inter, Addr addr, uint8_t val) {
  addr = mask_region(addr);

  int32_t offset = range_contains(range(RAM), addr);
  if (offset >= 0) {
    store_ram8(&inter->ram, MAKE_Addr(offset), val);
    return;
  }

  offset = range_contains(range(EXPANSION2), addr);
  if (offset >= 0) {
    log_error("Unhandled Write to EXPANSION2 registers. addr: 0x%08X. val: 0x%08X", addr, val);
    return;
  }

  fatal("Unhandled store8 call. addr: 0x%08X, val: 0x%08X", addr, val);
}

uint32_t get_dma_reg(Interconnect *inter, Addr offset) {
  uint8_t major = (offset.data & 0x70) >> 4;
  uint8_t minor = offset.data & 0xF;

  uint32_t return_val;

  switch (major) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
      {
        switch (minor) {
          case 0:
            return_val = inter->dma.channels[major].base_address.data;
            break;
          case 4:
            return_val = get_dma_channel_block_control(&inter->dma.channels[major]);
            break;
          case 8:
            return_val = get_dma_channel_control(&inter->dma.channels[major]);
            break;
          default:
            fatal("Unhandled DMA Read. addr: 0x%08X", offset);
        }
      }
      break;
    case 7:
      switch (minor) {
        case 0:
          return_val = inter->dma.control;
          break;
        case 4:
          return_val = get_dma_interrupt(&inter->dma);
          break;
        default:
          fatal("Unhandled DMA Read. addr: 0x%08X", offset);
      }
      break;
    default:
      fatal("Unhandled DMA Read. addr: 0x%08X", offset);
  }

  return return_val;
}

void set_dma_reg(Interconnect *inter, Addr offset, uint32_t val) {
  uint8_t major = (offset.data & 0x70) >> 4;
  uint8_t minor = offset.data & 0xF;

  switch (major) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
      {
        switch (minor) {
          case 0:
            set_dma_channel_base_address(&inter->dma.channels[major], val);
            break;
          case 4:
            set_dma_channel_block_control(&inter->dma.channels[major], val);
            break;
          case 8:
            set_dma_channel_control(&inter->dma.channels[major], val);
            break;
          default:
            fatal("Unhandled DMA Write. addr: 0x%08X, val: 0x%08X", offset, val);
        }

        if (get_dma_channel_active(&inter->dma.channels[major])) {
          perform_dma(inter, major);
        }
      }
      break;
    case 7:
      switch (minor) {
        case 0:
          inter->dma.control = val;
          break;
        case 4:
          set_dma_interrupt(&inter->dma, val);
          break;
        default:
          fatal("Unhandled DMA Write. addr: 0x%08X, val: 0x%08X", offset, val);
      }
      break;
    default:
      fatal("Unhandled DMA Write. addr: 0x%08X, val: 0x%08X", offset, val);
  }
}

void perform_dma_block(Interconnect *inter, DmaPort port) {
  START_LOGGING_PC();

  DmaChannel *channel = inter->dma.channels + port;
  int8_t increment = 4 - 8 * channel->step;
  Addr addr = channel->base_address;
  uint32_t transfer_size = get_dma_channel_transfer_size(channel);

  if (transfer_size == 0)
    fatal("perform_dma_block called in LinkedList Mode!");
  
  LOG_OUTPUT(inter->output_log_index, "DMA BLOCK COPY. Port: %x, Control: %08x, Dest: RAM, Addr: %08x, Size: %08x", port, get_dma_channel_control(channel), addr.data, transfer_size);
  print_output_log(inter->output_log_index);

  while (transfer_size > 0) {
    Addr cur_addr = MAKE_Addr(addr.data & 0x001FFFFC);

    switch (channel->direction) {
      case DmaFromRam:
        {
          uint32_t source_word = load_ram32(&inter->ram, cur_addr);
          switch (port) {
            case DmaGpu:
              gpu_gp0(&inter->gpu, source_word);
              break;
            default:
              fatal("Unhandled DMA Destination Port. port: 0x%08X", port);
          }
        }
        break;
      case DmaToRam:
        {
          uint32_t source_word;
          switch (port) {
            case DmaGpu:
              log_error("Unhandled DMA_GPU Port. port: 0x%08X", port);
            case DmaOtc:
              source_word = (transfer_size == 1) ? 0x00FFFFFF : ((addr.data - 4) & 0x001FFFFF);
              break;
            default:
              fatal("Unhandled DMA Port. port: 0x%08X", port);
          }

          LOG_OUTPUT(inter->output_log_index, "DMA BLOCK COPY: Addr: %08x, Data: %08x", cur_addr.data, source_word);
          print_output_log(inter->output_log_index);
          store_ram32(&inter->ram, cur_addr, source_word);
        }
    }

    addr = MAKE_Addr(addr.data + increment);
    transfer_size--;
  }
}

void perform_dma_linked_list(Interconnect *inter, DmaPort port) {
  DmaChannel *channel = inter->dma.channels + port;
  Addr addr = MAKE_Addr(channel->base_address.data & 0x001FFFFC);

  if (channel->direction == DmaToRam)
    fatal("Invalid DMA Direction LinkedList Mode");

  if (port != DmaGpu)
    fatal("Attempted LinkedList DMA on port: 0x%08X", port);

  while (1) {
    uint32_t header = load_ram32(&inter->ram, addr);
    uint32_t transfer_size = header >> 24;

    LOG_OUTPUT(inter->output_log_index, "DMA Linked List. Port: %x, Control: %08x, Dest: GPU, Addr: %08x, Size: %08x, Header: %08X", port, get_dma_channel_control(channel), addr.data, transfer_size, header);
    print_output_log(inter->output_log_index);

    while (transfer_size > 0) {
      addr = MAKE_Addr((addr.data + 4) & 0x001FFFFC);

      uint32_t command = load_ram32(&inter->ram, addr);

      gpu_gp0(&inter->gpu, command);

      transfer_size--;
    }

    if (header & 0x00800000)
      break;

    addr = MAKE_Addr(header & 0x001FFFFC);
  }
}

void perform_dma(Interconnect *inter, DmaPort port) {
  switch (inter->dma.channels[port].sync) {
    case DmaLinkedList:
      perform_dma_linked_list(inter, port);
      break;
    default:
      perform_dma_block(inter, port);
      break;
  }

  inter->dma.channels[port].trigger = false;
  inter->dma.channels[port].enable = false;
  // ToDo: Set correct values for other fields (i.e. interrupts)
}

void destroy_interconnect(Interconnect *inter) {
  destroy_bios(&inter->bios);
  destroy_ram(&inter->ram);
  destroy_gpu(&inter->gpu);
}
