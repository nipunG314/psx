#include <stdint.h>

#include "instruction.h"

#ifndef RANGE_H
#define RANGE_H

typedef struct Range {
  uint32_t start;
  uint32_t size;
} Range;

typedef enum RangeIndex {
  BIOS,
  MEM_CONTROL,
  RAM_SIZE,
  CACHE_CONTROL,
  RAM,
  SPU,
  EXPANSION1,
  EXPANSION2,
  IRQ,
  TIMERS,
  DMA,
  GPU,
  SCRATCH_PAD,
  PAD_MEMCARD,
  RANGE_COUNT
} RangeIndex;

static Range ranges[RANGE_COUNT] = {
  [BIOS] = {0x1FC00000, 512 * 1024},
  [MEM_CONTROL] = {0x1F801000, 36},
  [RAM_SIZE] = {0x1F801060, 4},
  [CACHE_CONTROL] = {0xFFFE0130, 4},
  [RAM] = {0x00000000, 8 * 1024 * 1024},
  [SPU] = {0x1F801C00, 640},
  [EXPANSION1] = {0x1F000000, 8 * 1024 * 1024},
  [EXPANSION2] = {0x1F802000, 66},
  [IRQ] = {0x1F801070, 8},
  [TIMERS] = {0x1F801100, 48},
  [DMA] = {0x1F801080, 0x80},
  [GPU] = {0x1F801810, 8},
  [SCRATCH_PAD] = {0x1F800000, 1024},
  [PAD_MEMCARD] = {0x1F801040, 32}
};

static inline Range range(RangeIndex index) {
  return ranges[index];
}

static inline int32_t range_contains(Range data_range, Addr addr) {
  if (addr.data >= data_range.start && addr.data < data_range.start + data_range.size) {
    if (data_range.start == range(RAM).start)
      addr = MAKE_Addr(addr.data & 0x1FFFFF);
    return addr.data - data_range.start;
  }

  return -1;
}

#endif
