#include <stdlib.h>
#include "clock.h"

PeripheralClock init_peripheral_clock() {
  PeripheralClock clock;
  clock.last_sync = MAKE_Cycles(0);
  clock.next_sync = MAKE_Cycles(0);

  return clock;
}

Cycles sync_peripheral(PeripheralClock *clock, Cycles now) {
  Cycles delta = MAKE_Cycles(now.data - clock->last_sync.data);
  clock->last_sync = now;
  return delta;
}

Clock init_clock() {
  Clock clock;
  clock.now = MAKE_Cycles(0);
  clock.next_sync = MAKE_Cycles(0);
  for(size_t i=0; i<PeripheralCount; i++)
    clock.peripheral_clocks[i] = init_peripheral_clock();

  return clock;
}

void set_next_sync_delta(Clock *clock, Peripheral peripheral, Cycles delta) {
  Cycles next = MAKE_Cycles(clock->now.data + delta.data);
  set_next_sync(&clock->peripheral_clocks[peripheral], next);
  if (next.data < clock->next_sync.data)
    clock->next_sync = next;
}

void update_sync_pending(Clock *clock) {
  Cycles new_next_sync = MAKE_Cycles(UINT64_MAX);

  for(size_t i = 0; i < PeripheralCount; i++) {
    Cycles cur_next_sync = clock->peripheral_clocks[i].next_sync;
    new_next_sync = (new_next_sync.data > cur_next_sync.data) ? cur_next_sync : new_next_sync;
  }
}
