#include <math.h>

#include "timers.h"
#include "log.h"

Timers init_timers() {
  Timers timers;

  for(size_t timer_index = 0; timer_index < 3; timer_index++)
    timers.timers[timer_index] = init_timer();
  timers.in_hsync = false;
  timers.in_vsync = false;
  timers.divider_8 = MAKE_Cycles(0);

  return timers;
}

uint32_t load_timers(Psx *psx, Addr offset, AddrType type) {
  run_timers(psx);

  Timer *timer = psx->timers.timers + (offset.data >> 4) & 0xF;

  switch (offset.data & 0xF) {
    case 0:
      return timer->counter.data;
    case 4:
      return read_timer_mode(timer);
    case 8:
      return timer->target;
    default:
      fatal("Unknown offset called in load_timers. Offset: 0x%08X, Type: %d", offset, type);
  }
}

void store_timers(Psx *psx, Addr offset, uint32_t val, AddrType type) {
  run_timers(psx);

  Timer *timer = psx->timers.timers + (offset.data >> 4) & 0xF;

  switch (offset.data & 0xF) {
    case 0:
      set_timer_counter(timer, val & 0xFFFF);
      return;
    case 4:
      configure_timer_mode(timer, val & 0xFFFF);
      return;
    case 8:
      timer->target = val & 0xFFFF;
      return;
    default:
      fatal("Unknown offset called in store_timers. Offset: 0x%08X, Type: %d", offset, type);
  }

  if (timer_target_match(timer) && handle_target_match(timer))
    trigger(psx, timer_irq[(offset.data >> 4) & 0xF]);

  predict_next_sync(&psx->timers);
}

bool next_irq(Timers *timers, Cycles *next_irq_delta) {
  bool found_irq = false;
  Cycles delta;

  for(size_t timer_index = 0; timer_index < 3; timer_index++) {
    if (next_timer_irq(timers->timers + timer_index, &delta, timer_index)) {
      if (get_timer_clock(timers->timers + timer_index, timer_index) == SystemClock8)
        delta.data = delta.data * 8 - timers->divider_8.data;

      if (found_irq)
        *next_irq_delta = MAKE_Cycles(fmin(next_irq_delta->data, delta.data));
      else
        *next_irq_delta = delta;
      found_irq = true;
    }
  }

  return found_irq;
}

bool run_cpu(Timers *timers, Cycles cycle_count, size_t timer_index) {
  ClockSource clock = get_timer_clock(timers->timers + timer_index, timer_index);

  // Check for Timer 2
  if (timer_index == 2) {
    timers->divider_8.data += cycle_count.data;
    Cycles ticks = MAKE_Cycles(timers->divider_8.data / 8);
    timers->divider_8.data &= 7;

    if (clock == SystemClock8)
      return run_timer(timers->timers + timer_index, ticks);
  }

  if (clock == GpuDotClock || clock == GpuHSyncClock) {
    fatal("run_cpu being called by GPU!");
  }

  return run_timer(timers->timers + timer_index, cycle_count);
}

void run_timers_helper(Psx *psx) {
  Cycles elapsed = resync(psx, SyncTimers); 

  for(size_t timer_index = 0; timer_index < 3; timer_index++) {
    if (run_cpu(&psx->timers, elapsed, timer_index))
      trigger(psx, timer_irq[timer_index]);
  }
}

Timer init_timer() {
  Timer timer;

  timer.mode = init_timer_mode();
  timer.counter = MAKE_Cycles(0);
  timer.target = 0;
  timer.irq_disable = false;
  timer.sync_state = SyncRunning;

  return timer;
}

bool handle_target_match(Timer *timer) {
  timer->mode.reached_target = true;

  if (timer->mode.reset_on_target) {
    if (timer->target == 0)
      timer->counter = MAKE_Cycles(0);
    else
      timer->counter.data %= timer->target;
  }

  if (timer->mode.irq_on_target && !timer->irq_disable) {
    if (!timer->mode.irq_repeat)
      timer->irq_disable = true;
    return true;
  }

  return false;
}

bool handle_overflow_match(Timer *timer) {
  timer->mode.reached_overflow = true;

  timer->counter.data &= 0xFFFF;

  if (timer->mode.irq_on_overflow && !timer->irq_disable) {
    if (!timer->mode.irq_repeat)
      timer->irq_disable = true;
    return true;
  }

  return false;
}

bool next_timer_irq(Timer *timer, Cycles *next_irq_delta, size_t timer_index) {
  TimerMode *mode = &timer->mode;
  if (!(mode->irq_on_target || mode->irq_on_overflow) || timer->irq_disable)
    return false;

  SyncMode sync_mode = get_timer_sync_mode(timer, timer_index);
  ClockSource clock_source = get_timer_clock(timer, timer_index);

  // Handle GPU Timings from GPU
  if (clock_source == GpuDotClock || clock_source == GpuHSyncClock)
    return false;

  if (sync_mode == Stopped)
    return false;

  int32_t next = 0;
  if (timer->target > timer->counter.data) {
    next = 0x10000;
  } else if (timer->mode.irq_on_target || timer->mode.reset_on_target) {
    next = timer->target;
  } else {
    next = 0x10000;
  }

  int32_t delta = next - timer->counter.data;
  if (delta < 1)
    delta = 1;
  next_irq_delta->data = delta;

  return true;
}

bool run_timer(Timer *timer, Cycles cycles) {
  if (timer->target == 0 && timer->mode.reset_on_target && timer_target_match(timer))
    return handle_target_match(timer);

  if (cycles.data == 0 || timer->sync_state != SyncRunning)
    return false;

  int32_t prev_counter = timer->counter.data;
  timer->counter.data += cycles.data;

  bool target_irq = (prev_counter < timer->target && timer->counter.data >= timer->target) ? handle_target_match(timer) : false;
  bool overflow_irq = (timer->counter.data > 0xFFFF) ? handle_overflow_match(timer) : false;

  return target_irq || overflow_irq;
}

TimerMode init_timer_mode() {
  TimerMode mode;

  mode.sync_enable = false;
  mode.sync_mode_config = 0;
  mode.reset_on_target = false;
  mode.irq_on_target = false;
  mode.irq_on_overflow = false;
  mode.irq_repeat = false;
  mode.irq_toggle_mode = false;
  mode.clock_source_config = 0;
  mode.reached_target = false;
  mode.reached_overflow = false;

  return mode;
}

void set_timer_mode(TimerMode *mode, uint16_t val) {
  mode->sync_enable = val & 1;
  mode->sync_mode_config = (val >> 1) & 3;
  mode->reset_on_target = (val >> 3) & 1;
  mode->irq_on_target = (val >> 4) & 1;
  mode->irq_on_overflow = (val >> 5) & 1;
  mode->irq_repeat = (val >> 6) & 1;
  mode->irq_toggle_mode = (val >> 7) & 1;
  mode->clock_source_config = (val >> 8) & 3;
}

uint16_t get_timer_mode(TimerMode *mode) {
  uint16_t res = 0;

  res |= mode->sync_enable;
  res |= (mode->sync_mode_config & 3) << 1;
  res |= (1 << 3) * mode->reset_on_target;
  res |= (1 << 4) * mode->irq_on_target;
  res |= (1 << 5) * mode->irq_on_overflow;
  res |= (1 << 6) * mode->irq_repeat;
  res |= (1 << 7) * mode->irq_toggle_mode;
  res |= ((uint16_t)mode->clock_source_config & 3) << 8;

  return res;
}
