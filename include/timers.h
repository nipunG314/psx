#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "types.h"
#include "irq.h"

#ifndef TIMERS_H
#define TIMERS_H

static Irq timer_irq[] = {
  Timer0Interrupt,
  Timer1Interrupt,
  Timer2Interrupt
};

typedef enum SyncMode {
  FreeRun,
  Stopped,
  PauseOnHSync,
  ResetOnHSync,
  HSyncOnly,
  FreeRunAfterHSync,
  PauseOnVSync,
  ResetOnVSync,
  VSyncOnly,
  FreeRunAfterVSync,
  SyncModeCount
} SyncMode;

static SyncMode timer_sync_modes[3][4] = {
  { PauseOnHSync, ResetOnHSync, HSyncOnly, FreeRunAfterHSync },
  { PauseOnVSync, ResetOnVSync, VSyncOnly, FreeRunAfterVSync },
  { Stopped, FreeRun, Stopped, FreeRun }
};

typedef enum ClockSource {
  SystemClock,
  GpuDotClock,
  GpuHSyncClock,
  SystemClock8,
} ClockSource;

static ClockSource timer_clock_source[3][4] = {
  { SystemClock, GpuDotClock, SystemClock, GpuDotClock },
  { SystemClock, GpuHSyncClock, SystemClock, GpuHSyncClock },
  { SystemClock, SystemClock, SystemClock8, SystemClock8 },
};

typedef struct TimerMode {
  bool sync_enable;
  uint8_t sync_mode_config;
  bool reset_on_target;
  bool irq_on_target;
  bool irq_on_overflow;
  bool irq_repeat;
  bool irq_toggle_mode;
  uint8_t clock_source_config;
  // READ ONLY
  bool reached_target;
  bool reached_overflow;
} TimerMode;

TimerMode init_timer_mode();
void set_timer_mode(TimerMode *mode, uint16_t val);
uint16_t get_timer_mode(TimerMode *mode);

typedef enum SyncState {
  SyncRunning,
  SyncPaused,
  SyncWaitingForStart,
  SyncWaitingForEnd
} SyncState;

typedef struct Timer {
  TimerMode mode;
  Cycles counter;
  uint16_t target;
  bool irq_disable;
  SyncState sync_state;
} Timer;

Timer init_timer();

static inline bool timer_target_match(Timer *timer) {
  return timer->target == timer->counter.data;
}

static inline void set_timer_counter(Timer *timer, uint16_t val) {
  timer->counter = MAKE_Cycles(val);
  timer->irq_disable = false;
}

static inline SyncMode get_timer_sync_mode(Timer *timer, size_t timer_index) {
  if (!timer->mode.sync_enable)
    return FreeRun;

  return timer_sync_modes[timer_index][timer->mode.sync_mode_config & 3]; 
}

static inline void configure_timer_mode(Timer *timer, uint16_t mode) {
  set_timer_mode(&timer->mode, mode);
  set_timer_counter(timer, 0);
  timer->sync_state = SyncPaused;
}

static inline uint16_t read_timer_mode(Timer *timer) {
  uint16_t res = get_timer_mode(&timer->mode);
  timer->mode.reached_overflow = false;
  if (!timer_target_match(timer))
    timer->mode.reached_target = false;
  return res;
}

bool handle_target_match(Timer *timer);
bool handle_overflow_match(Timer *timer);
bool next_irq(Timer *timer, Cycles *next_irq_delta, size_t timer_index);
bool run_timer(Timer *timer, Cycles cycles);

static SyncState sync_mode_sync_state_lut[SyncModeCount][4] = {
  [FreeRun] = {SyncRunning, SyncRunning, SyncRunning, SyncRunning},
  [Stopped] = {SyncPaused, SyncPaused, SyncPaused, SyncPaused},
  [PauseOnHSync] = {SyncRunning, SyncPaused, SyncRunning, SyncPaused},
  [ResetOnHSync] = {SyncRunning, SyncRunning, SyncRunning, SyncRunning},
  [HSyncOnly] = {SyncPaused, SyncRunning, SyncPaused, SyncRunning},
  [FreeRunAfterHSync] = {SyncWaitingForStart, SyncWaitingForStart, SyncWaitingForStart, SyncWaitingForStart},
  [PauseOnVSync] = {SyncRunning, SyncRunning, SyncPaused, SyncPaused},
  [ResetOnVSync] = {SyncRunning, SyncRunning, SyncRunning, SyncRunning},
  [VSyncOnly] = {SyncPaused, SyncPaused, SyncRunning, SyncRunning},
  [FreeRunAfterVSync] = {SyncWaitingForStart, SyncWaitingForStart, SyncWaitingForStart, SyncWaitingForStart}
};

static inline void refresh_timer_sync(Timer *timer, bool in_hsync, bool in_vsync, size_t timer_index) {
  SyncMode sync_mode = get_timer_sync_mode(timer, timer_index);

  // If timer is waiting for sync to end (WaitingForSyncEnd), that will handled from the GPU
  // If timer is already Running, we don't want to change that
  if ((sync_mode == FreeRunAfterHSync || sync_mode == FreeRunAfterVSync) && timer->sync_state != SyncPaused)
    return;

  timer->sync_state = sync_mode_sync_state_lut[sync_mode][2 * in_vsync + in_hsync];
}

#endif
