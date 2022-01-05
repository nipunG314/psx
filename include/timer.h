#include <stdint.h>
#include <stdbool.h>

#include "instruction.h"
#include "clock.h"
#include "shared.h"
#include "gpu.h"

#ifndef TIMER_H
#define TIMER_H

typedef enum SyncMode {
  Pause,
  Reset,
  ResetAndPause,
  WaitForSync
} SyncMode;

DECLARE_TYPE(uint8_t, ClockSource)
typedef enum ClockType {
  SysClock,
  SysClockDiv8,
  GpuPixelClock,
  GpuHSync
} ClockType;

ClockType const clock_type_lookup[3][4] = {
  // Timer0
  {SysClock, GpuPixelClock, SysClock, GpuPixelClock},
  // Timer1
  {SysClock, GpuHSync, SysClock, GpuHSync},
  // Timer2
  {SysClock, SysClock, SysClockDiv8, SysClockDiv8},
};

static inline ClockType clock_type(ClockSource source, Peripheral peripheral) {
  return clock_type_lookup[peripheral - Timer0][source.data];  
}

typedef struct Timer {
  Peripheral timer_instance;
  uint16_t counter;
  uint16_t target;
  bool use_sync;
  SyncMode sync_mode;
  bool target_wrap;
  bool target_irq;
  bool wrap_irq;
  bool repeat_irq;
  bool negate_irq;
  ClockSource clock_source;
  bool target_reached;
  bool overflow_reached;
  FracCycles period;
  FracCycles phase;
  bool interrupt;
} Timer;

Timer init_timer(Peripheral peripheral);
void reconfigure(Timer *timer, SharedState *shared, Gpu *gpu);
void timer_sync(Timer *timer, SharedState *shared);
void predict_next_sync(Timer *timer, SharedState *shared);
uint16_t get_timer_mode(Timer *timer);
void set_timer_mode(Timer *timer, uint16_t val);
bool needs_gpu(Timer *timer);

typedef struct Timers {
  Timer timers[3];
} Timers;

Timers init_timers();
uint32_t load_timers(Timers *timers, SharedState *shared, Addr offset, AddrType type);
void store_timers(Timers *timers, SharedState *shared, Gpu *gpu, Addr offset, uint32_t val, AddrType type);
void timers_sync(Timers *timers, SharedState *shared);

#endif
