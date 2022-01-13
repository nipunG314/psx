#include <stdint.h>
#include <stdbool.h>

#ifndef IRQ_H
#define IRQ_H

typedef struct Psx Psx;

typedef enum Irq {
  VblankInterrupt,
  GpuInterrupt,
  CDRomInterrupt,
  DmaInterrupt,
  Timer0Interrupt,
  Timer1Interrupt,
  Timer2Interrupt,
  PadMemCardInterrupt,
  SioInterrupt,
  SpuInterrupt
} Irq;

typedef struct IrqState {
  uint16_t status;
  uint16_t mask;
} IrqState;

IrqState init_irq();

static inline bool irq_active(IrqState *irq) {
  return irq->status & irq->mask;
}

void set_irq_mask(Psx *psx, uint16_t mask);
void trigger(Psx *psx, Irq interrupt);
void ack(Psx *psx, uint16_t ack);

#endif
