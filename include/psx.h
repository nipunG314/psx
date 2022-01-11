#include <stdint.h>

#include "types.h"

#ifndef PSX_H
#define PSX_H
typedef struct Psx {
  Cycles cycles_counter;
} Psx;

Psx init_psx(char const *bios_file_path);

static inline void tick(Psx *psx, Cycles cycle_count) {
  psx->cycles_counter.data += cycle_count.data;
}

#endif
