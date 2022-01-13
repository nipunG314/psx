#include "psx.h"
#include "irq.h"

IrqState init_irq() {
  IrqState irq;

  irq.mask = 0;
  irq.status = 0;
  
  return irq;
}

void set_irq_mask(Psx *psx, uint16_t mask) {
  psx->irq.mask = mask;

  handle_irq_changed(psx);
}

void trigger(Psx *psx, Irq interrupt) {
  psx->irq.status |= 1 << interrupt;

  handle_irq_changed(psx);
}

void ack(Psx *psx, uint16_t ack) {
  psx->irq.status &= ack;

  handle_irq_changed(psx);
}
