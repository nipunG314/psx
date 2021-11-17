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
  EXPANSION2,
  RANGE_COUNT
} RangeIndex;

static Range ranges[RANGE_COUNT] = {
  [BIOS] = {0x1FC00000, 512 * 1024},
  [MEM_CONTROL] = {0x1F801000, 36},
  [RAM_SIZE] = {0x1F801060, 4},
  [CACHE_CONTROL] = {0xFFFE0130, 4},
  [RAM] = {0x00000000, 2 * 1024 * 1024},
  [SPU] = {0x1F801C00, 640},
  [EXPANSION2] = {0x1F802000, 66}
};

static inline Range range(RangeIndex index) {
  return ranges[index];
}

static inline int32_t range_contains(Range range, Addr addr) {
  if (addr.data >= range.start && addr.data < range.start + range.size)
    return addr.data - range.start;

  return -1;
}

#endif
