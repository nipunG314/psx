#include "dma.h"

Dma init_dma() {
  Dma dma;
  dma.control = 0x07654321;

  return dma;
}
