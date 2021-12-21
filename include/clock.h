#include <stdint.h>
#include <stdbool.h>

#include "instruction.h"

#ifndef CLOCK_H
#define CLOCK_H

DECLARE_TYPE(uint64_t, Cycles)
DECLARE_TYPE(uint64_t, FracCycles)

#define FRAC_BITS_COUNT 16

static inline FracCycles add_frac_cycles(FracCycles f1, FracCycles f2) {
  return MAKE_FracCycles(f1.data + f2.data);
}

static inline FracCycles multiply_frac_cycles(FracCycles f1, FracCycles f2) {
  uint64_t mult = f1.data * f2.data;
  return MAKE_FracCycles(mult >> FRAC_BITS_COUNT);
}

static inline FracCycles divide_frac_cycles(FracCycles f1, FracCycles f2) {
  uint64_t num = f1.data << FRAC_BITS_COUNT;
  return MAKE_FracCycles(num / f2.data);
}

static inline Cycles ceil_frac_cycles(FracCycles f) {
  uint64_t shift = (1 << FRAC_BITS_COUNT) - 1;
  return MAKE_Cycles((f.data + shift) >> FRAC_BITS_COUNT);
}

typedef enum Peripheral {
  Timer0,
  Timer1,
  Timer2,
  PeripheralCount 
} Peripheral;

typedef struct PeripheralClock {
  Cycles last_sync;
  Cycles next_sync;
} PeripheralClock;

PeripheralClock init_peripheral_clock();
Cycles sync_peripheral(PeripheralClock *clock, Cycles now);
static inline void set_next_sync(PeripheralClock *clock, Cycles next) {
  clock->next_sync = next;
}
static inline bool needs_sync_peripheral(PeripheralClock *clock, Cycles now) {
  return clock->next_sync.data <= now.data;
}

typedef struct Clock {
  Cycles now;
  Cycles next_sync;
  PeripheralClock peripheral_clocks[PeripheralCount];
} Clock;

Clock init_clock();
static inline void tick(Clock* clock, Cycles secs) {
  clock->now.data += secs.data;
}
static inline Cycles sync(Clock *clock, Peripheral peripheral) {
  return sync_peripheral(&clock->peripheral_clocks[peripheral], clock->now);
}

void set_next_sync_delta(Clock *clock, Peripheral peripheral, Cycles delta);

static inline void no_sync_needed(Clock *clock, Peripheral peripheral) {
  set_next_sync(&clock->peripheral_clocks[peripheral], MAKE_Cycles(UINT64_MAX));
}

static inline bool needs_sync(Clock *clock, Peripheral peripheral) {
  return  needs_sync_peripheral(&clock->peripheral_clocks[peripheral], clock->now); 
}

static inline bool sync_pending(Clock *clock) {
  return clock->next_sync.data <= clock->now.data;
}

void update_sync_pending(Clock *clock);

#endif
