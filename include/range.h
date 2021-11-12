#include <stdint.h>

#include "instruction.h"

#ifndef RANGE_H
#define RANGE_H

#define BIOS_START 0xBFC00000
#define BIOS_SIZE 512 * 1024

#define MEM_CONTROL_START 0x1F801000
#define MEM_CONTROL_SIZE 36

#define RAM_SIZE_START 0x1F801060
#define RAM_SIZE_SIZE 4

inline int32_t range_contains(uint32_t start, uint32_t size, Addr addr) {
  if (addr.data >= start && addr.data < start + size)
    return addr.data - start;

  return -1;
}

#endif
