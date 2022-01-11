#include <stdint.h>

#ifndef TYPES_H
#define TYPES_H

#define DECLARE_TYPE(T, N) typedef struct N {\
  T data;\
} N;\
static inline N MAKE_##N(T x) {\
  N val = {x};\
  return val;\
}

DECLARE_TYPE(uint32_t, Addr)
DECLARE_TYPE(int32_t, Cycles)

typedef enum AddrType {
  AddrByte = 1,
  AddrHalf = 2,
  AddrWord = 4
} AddrType;

static uint32_t const AddrRegionMask[] = {
  // KUSEG - 2GB
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
  // KSEG0 - 512MB
  0x7FFFFFFF,
  // KSEG1 - 512MB
  0x1FFFFFFF,
  // KSEG2 - 1GB
  0xFFFFFFFF, 0xFFFFFFFF
};

static inline Addr mask_region(Addr addr) {
  uint8_t index = addr.data >> 29;

  return MAKE_Addr(addr.data & AddrRegionMask[index]);
}

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

typedef struct Range {
  uint32_t start;
  uint32_t size;
} Range;

static Range const ranges[RANGE_COUNT] = {
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
