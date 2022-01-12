#include <stdlib.h>

#include "psx.h"
#include "log.h"

/*
void side_load_rom(Cpu *cpu) {
  FILE * const fp = fopen(get_rom_filename(), "rb");

  if (!fp)
    fatal("IOError: Could not open ROM: %s", rom_filename);

  RomHeader header;

  fseek(fp, 0x10L, SEEK_SET);
  if (fread(&header, sizeof(RomHeader), 1, fp) != 1)
    fatal("IOError: Invalid ROM Header: %s", rom_filename);

  cpu->next_pc = header.initial_pc;
  cpu->regs[28] = header.initial_r28;
  header.ram_address = mask_region(header.ram_address);

  fseek(fp, 0x0L, SEEK_END);
  uint64_t count = ftell(fp) - 1;
  fseek(fp, 0x800L, SEEK_SET);
  count -= ftell(fp);

  if (fread(&cpu->inter.ram.data[header.ram_address.data], sizeof(uint8_t), count, fp) != sizeof(uint8_t) * count)
    fatal("IOError: Couldn't read ROM data: %s", rom_filename);

  memset(&cpu->inter.ram.data[header.data_start.data], 0, header.data_size);
  memset(&cpu->inter.ram.data[header.bss_start.data], 0, header.bss_size);
  if (header.r29_start.data) {
    cpu->regs[29] = header.r29_start.data + header.r29_size;
    cpu->regs[30] = header.r29_start.data + header.r29_size;
  }

  fclose(fp);
}
*/

Psx init_psx(char const *bios_file_path) {
  Psx psx;

  psx.cycles_counter = MAKE_Cycles(0);
  psx.dma_timing_penalty = MAKE_Cycles(0);
  psx.bios = init_bios(bios_file_path); 
  psx.ram = init_ram();
  psx.cpu = init_cpu();
  psx.cop0 = init_cop0();

  return psx;
}

uint32_t load(Psx *psx, Addr addr, AddrType type) {
  addr = mask_region(addr);

  tick(psx, psx->dma_timing_penalty);
  psx->dma_timing_penalty.data = 0;

  int32_t offset = range_contains(range(BIOS), addr);
  if (offset >= 0) {
    return load_bios(&psx->bios, MAKE_Addr(offset), type);
  }

  offset = range_contains(range(RAM), addr);
  if (offset >= 0) {
    tick(psx, MAKE_Cycles(3));
    return load_ram(&psx->ram, MAKE_Addr(offset), type);
  }

  offset = range_contains(range(SPU), addr);
  if (offset >= 0) {
    tick(psx, MAKE_Cycles(type == AddrWord ? 36 : 16));
    log_error("Unhandled read from SPU. addr: 0x%08X, type: %d", addr, type);
    return 0;
  }

  offset = range_contains(range(DMA), addr);
  if (offset >= 0) {
    tick(psx, MAKE_Cycles(1));
    log_error("Unhandled read from DMA. addr: 0x%08X, type: %d", addr, type);
    return 0;
  }

  offset = range_contains(range(TIMERS), addr);
  if (offset >= 0) {
    tick(psx, MAKE_Cycles(1));
    log_error("Unhandled read from TIMERS. addr: 0x%08X, type: %d", addr, type);
    return 0;
  }

  offset = range_contains(range(GPU), addr);
  if (offset >= 0) {
    tick(psx, MAKE_Cycles(1));
    log_error("Unhandled read from GPU. addr: 0x%08X, type: %d", addr, type);
    return 0;
  }

  offset = range_contains(range(PAD_MEMCARD), addr);
  if (offset >= 0) {
    tick(psx, MAKE_Cycles(1));
    log_error("Unhandled read from PAD_MEMCARD. addr: 0x%08X, type: %d", addr, type);
    return 0;
  }

  offset = range_contains(range(EXPANSION1), addr);
  if (offset >= 0)
    return ~0;

  fatal("Unhandled load call. Address: 0x%08X, Type: %d", addr, type);
}

void store(Psx *psx, Addr addr, uint32_t val, AddrType type) {
  addr = mask_region(addr);

  int32_t offset = range_contains(range(RAM), addr);
  if (offset >= 0) {
    store_ram(&psx->ram, MAKE_Addr(offset), val, type);
    return;
  }

  offset = range_contains(range(SCRATCH_PAD), addr);
  if (offset >= 0) {
    log_error("Unhandled write to SCRATCH_PAD. addr: 0x%08X, val: 0x%08X, type: %d", addr, val, type);
    return;
  }

  offset = range_contains(range(SPU), addr);
  if (offset >= 0) {
    log_error("Unhandled write to SCRATCH_PAD. addr: 0x%08X, val: 0x%08X, type: %d", addr, val, type);
    return;
  }

  offset = range_contains(range(DMA), addr);
  if (offset >= 0) {
    log_error("Unhandled write to SCRATCH_PAD. addr: 0x%08X, val: 0x%08X, type: %d", addr, val, type);
    return;
  }

  offset = range_contains(range(TIMERS), addr);
  if (offset >= 0) {
    log_error("Unhandled write to SCRATCH_PAD. addr: 0x%08X, val: 0x%08X, type: %d", addr, val, type);
    return;
  }

  offset = range_contains(range(GPU), addr);
  if (offset >= 0) {
    log_error("Unhandled write to SCRATCH_PAD. addr: 0x%08X, val: 0x%08X, type: %d", addr, val, type);
    return;
  }

  fatal("Unhandled write call. Address: 0x%08X, Type: %d", addr, type);
}
