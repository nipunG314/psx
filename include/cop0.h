#include <stdint.h>
#include <stdbool.h>

#include "types.h"

#ifndef COP0_H
#define COP0_H

typedef struct Psx Psx;

typedef enum Exception {
  Interrupt = 0x0,
  SysCall = 0x8,
  Overflow = 0xc,
  LoadAddressError = 0x4,
  StoreAddressError = 0x5,
  Break = 0x9,
  CoprocessorError = 0xb,
  IllegalInstruction = 0xa
} Exception;

typedef struct Cop0 {
  uint32_t sr;
  uint32_t cause;
  uint32_t epc;
} Cop0;

Cop0 init_cop0();
uint32_t cause(Psx *psx);

static inline bool cache_isolated(Cop0 *cop0) {
  return cop0->sr & 0x10000;
}

static inline bool irq_enabled(Cop0 *cop0) {
  return cop0->sr & 1;
}

static inline bool irq_pending(Psx *psx) {
  // ToDo: Return true if irq_pending
  return false;
}

void mtc0(Psx *psx, RegIndex index, uint32_t val);
uint32_t mfc0(Psx *psx, RegIndex index);

Addr enter_exception(Psx *psx, Exception cause);
void return_from_exception(Psx *psx);

#endif
