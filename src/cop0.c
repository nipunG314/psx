#include <stdlib.h>

#include "psx.h"
#include "cop0.h"
#include "log.h"

Cop0 init_cop0() {
  Cop0 cop0;

  cop0.sr = 0;
  cop0.cause = 0;
  cop0.epc = 0;

  return cop0;
}

uint32_t cause(Psx *psx) {
  // ToDo: Add the IRQ bit
  return psx->cop0.cause;
}

void mtc0(Psx *psx, RegIndex index, uint32_t val) {
  switch (index.data) {
    case 3:
    case 5:
    case 6:
    case 7:
    case 9:
    case 11:
      if (val != 0)
        log_error("Unhandled write to cop0. Index: 0x%08X, Value: 0x%08X", index.data, val);
      break;
    case 12:
      psx->cop0.sr = val;
      handle_irq_changed(psx);
      break;
    case 13:
      psx->cop0.cause &= ~0x300;
      psx->cop0.cause |= val & 0x300;
      handle_irq_changed(psx);
      break;
    default:
      fatal("Unhandled write to cop0. Index: 0x%08X, Value: 0x%08X", index.data, val);
  }
}

uint32_t mfc0(Psx *psx, RegIndex index) {
  switch (index.data) {
    case 6:
      log_error("");
      return 0;
    case 7:
      log_error("");
      return 0;
    case 8:
      log_error("");
      return 0;
    case 12:
      return psx->cop0.sr;
    case 13:
      return cause(psx);
    case 14:
      return psx->cop0.epc;
    case 15:
      // Processor ID
      return 2;
    default:
      fatal("Unhandld read from cop0. Index: 0x%08X", index.data);
  }
}

Addr enter_exception(Psx *psx, Exception cause) {
  uint32_t pc = psx->cpu.current_pc.data;
  uint32_t mode = psx->cop0.sr & 0x3F;

  psx->cop0.sr &= ~0x3F;
  psx->cop0.sr |= (mode << 2) & 0x3F;

  psx->cop0.cause &= ~0x7C;
  psx->cop0.cause |= psx->cop0.cause << 2;

  if (psx->cpu.delay_slot) {
    psx->cop0.epc = pc - 4;
    psx->cop0.cause |= 1 << 31;
  } else {
    psx->cop0.epc = pc;
    psx->cop0.cause &= ~(1 << 31);
  }

  handle_irq_changed(psx);

  return MAKE_Addr((psx->cop0.sr & (1 << 22)) ? 0xBFC00180 : 0x80000080);
}

void return_from_exception(Psx *psx) {
  uint32_t mode = psx->cop0.sr & 0x3F;

  psx->cop0.sr &= ~0xF;
  psx->cop0.sr |= mode >> 2;

  handle_irq_changed(psx);
}
