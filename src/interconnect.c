#include <stdlib.h>

#include "instruction.h"
#include "interconnect.h"
#include "log.h"

uint32_t const RegionMask[] = {
  // KUSEG - 2GB
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
  // KSEG0 - 512MB
  0x7FFFFFFF,
  // KSEG1 - 512MB
  0x1FFFFFFF,
  // KSEG2 - 1GB
  0xFFFFFFFF, 0xFFFFFFFF
};

Addr mask_region(Addr addr) {
  uint8_t index = addr.data >> 29;

  return MAKE_Addr(addr.data & RegionMask[index]);
}

Interconnect init_interconnect(char const *bios_filename) {
  Interconnect inter = {0};

  inter.bios = init_bios(bios_filename);
  inter.ram = init_ram();
  inter.dma = init_dma();

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
    log_error("Unhandled read from GPU. addr: 0x%08X", addr);
    // TempFix: Return 0x10000000 when offset == 4
    // Tells BIOS that its ready DMA transfer
    return (offset == 4 ? 0x1C000000 : 0);
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
    log_error("Unhandled Write to GPU register. addr: 0x%08X. val: 0x%08X", addr, val);
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
            return inter->dma.channels[major].base_address.data;
          case 4:
            return get_dma_channel_block_control(&inter->dma.channels[major]);
          case 8:
            return get_dma_channel_control(&inter->dma.channels[major]);
          default:
            fatal("Unhandled DMA Read. addr: 0x%08X", offset);
        }
      }
      break;
    case 7:
      switch (minor) {
        case 0:
          return inter->dma.control;
        case 4:
          return get_dma_interrupt(&inter->dma);
        default:
          fatal("Unhandled DMA Read. addr: 0x%08X", offset);
      }
      break;
    default:
      fatal("Unhandled DMA Read. addr: 0x%08X", offset);
  }
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
          perform_dma(inter, &inter->dma.channels[major], major);
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

void perform_dma_block(Interconnect *inter, DmaChannel *channel, DmaPort port) {
  int8_t _increment = 8 * channel->step - 4;
  uint8_t increment = increment;
  Addr addr = channel->base_address;
  size_t transfer_size = get_dma_channel_transfer_size(channel);

  if (transfer_size == 0)
    fatal("perform_dma_block called in LinkedList Mode!");

  while (transfer_size > 0) {
    Addr cur_addr = MAKE_Addr(addr.data & 0x001FFFFC);

    switch (channel->direction) {
      case FromRam:
        fatal("Unhandled DMA Direction. direction: 0x%08X", channel->direction);
      case ToRam:
        {
          uint32_t source_word;
          switch (port) {
            case Otc:
              source_word = (transfer_size == 1) ? 0x00FFFFFF : ((addr.data - 4) & 0x001FFFFF);
              break;
            default:
              fatal("Unhandled DMA Port. port: 0x%08X", port);
          }

          store_ram32(&inter->ram, cur_addr, source_word);
        }
    }

    addr = MAKE_Addr(addr.data + increment);
    transfer_size--;
  }

  channel->enable = 0;
  channel->trigger = 0;
}

void perform_dma(Interconnect *inter, DmaChannel *channel, DmaPort port) {
  switch (channel->sync) {
    case LinkedList:
      fatal("LinkedList Mode Unsupported");
    default:
      perform_dma_block(inter, channel, port);
  }
}

void destroy_interconnect(Interconnect *inter) {
  destroy_bios(&inter->bios);
  destroy_ram(&inter->ram);
}
