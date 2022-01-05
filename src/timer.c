#include "timer.h"

Timer init_timer(Peripheral peripheral) {
  Timer timer;

  timer.timer_instance = peripheral;
  timer.counter = 0;
  timer.target = 0;
  timer.use_sync = false;
  timer.sync_mode = Pause;
  timer.target_wrap = false;
  timer.target_irq = false;
  timer.wrap_irq = false;
  timer.repeat_irq = false;
  timer.negate_irq = false;
  timer.clock_source = MAKE_ClockSource(0);
  timer.target_reached = false;
  timer.overflow_reached = false;
  timer.period = MAKE_FracCycles(0);
  timer.phase = MAKE_FracCycles(0);
  timer.interrupt = false;

  return timer;
}

/// Recomputes the entire timer's internal state. Must be called
/// when the timer's config changes *or* when the timer relies on
/// the GPU's video timings and those timings change.
///
/// If the GPU is needed for the timings it must be synchronized
/// before this function is called.
void reconfigure(Timer *timer, SharedState *shared, Gpu *gpu) {
  switch (clock_type(timer->clock_source, timer->timer_instance)) {
    case SysClock:
      timer->period = MAKE_FracCycles(1);
      timer->phase = MAKE_FracCycles(0);
      break;
    case SysClockDiv8:
      timer->period = MAKE_FracCycles(8);
      timer->phase = MAKE_FracCycles(0);
      break;
    default:
      log_error("GPU Timings Unhandled!");
  }

  predict_next_sync(timer, shared);
}

void timer_sync(Timer *timer, SharedState *shared) {
  Cycles delta = sync(&shared->clock, timer->timer_instance);
  if (delta.data == 0) {
    // The interrupt code below might glitch if it's called
    // two times in a row (trigger the interrupt twice). It
    // probably wouldn't be too dangerous but I'd rather keep it clean.
    return;
  }

  FracCycles delta_frac = MAKE_FracCycles(delta.data << FRAC_BITS_COUNT);
  FracCycles ticks = add_frac_cycles(delta_frac, timer->phase);
  uint16_t count = ticks.data / timer->period.data;
  uint16_t phase = ticks.data % timer->period.data;

  timer->phase = MAKE_FracCycles(phase);
  count += timer->counter;
  bool target_passed = false;
  if ((timer->counter <= timer->target) && (count > timer->target)) {
    timer->target_reached = true;
    target_passed = true;
  }
  uint32_t wrap;
  if (timer->target_wrap)
    wrap = timer->target + 1;
  else
    wrap = 0x10000;

  bool overflow = false;
  if (count >= wrap) {
    count = count % wrap;
    if (wrap == 0x10000) {
      timer->overflow_reached = true;
      overflow = true;
    }
  }

  timer->counter = count;
  if ((timer->wrap_irq && overflow) || (timer->target_irq && target_passed)) {
    fatal("Unhandled Interrupt!");
  } else if (!timer->negate_irq)
    timer->interrupt = false;

  predict_next_sync(timer, shared);
}

void predict_next_sync(Timer *timer, SharedState *shared) {
  // ToDo: Add support for wrap IRQ
  
  if (!timer->target_irq) {
    no_sync_needed(&shared->clock, timer->timer_instance);
    return;
  }

  uint16_t _countdown = timer->target - timer->counter;
  Cycles countdown = MAKE_Cycles((_countdown >= 0) ? _countdown : (0xFFFF - _countdown));
  FracCycles delta = MAKE_FracCycles(timer->period.data * (countdown.data + 1) - timer->phase.data);
  // Round up to the next CPU cycle
  Cycles delta_cycles = ceil_frac_cycles(delta);

  set_next_sync_delta(&shared->clock, timer->timer_instance, delta_cycles);
}

uint16_t get_timer_mode(Timer *timer) {
  uint16_t res = 0;

  res |= timer->use_sync;
  res |= timer->sync_mode << 1;
  res |= timer->target_wrap << 3;
  res |= timer->target_irq << 4;
  res |= timer->wrap_irq << 5;
  res |= timer->repeat_irq << 6;
  res |= timer->negate_irq << 7;
  res |= (timer->clock_source.data & 3) << 8;
  // Interrupt is active when low
  res |= (!timer->interrupt) << 10;
  res |= timer->target_reached << 11;
  res |= timer->overflow_reached << 12;

  // Reset flags
  timer->target_reached = false;
  timer->overflow_reached = false;

  return res;
}

void set_timer_mode(Timer *timer, uint16_t val) {
  timer->use_sync = val & 1;
  switch((val >> 1) & 3) {
    case 0:
      timer->sync_mode = Pause;
      break;
    case 1:
      timer->sync_mode = Reset;
      break;
    case 2:
      timer->sync_mode = ResetAndPause;
      break;
    case 3:
      timer->sync_mode = WaitForSync;
    break;
  }
  timer->target_wrap = (val >> 3) & 1;
  timer->target_irq = (val >> 4) & 1;
  timer->wrap_irq = (val >> 5) & 1;
  timer->repeat_irq = (val >> 6) & 1;
  timer->negate_irq = (val >> 7) & 1;
  timer->clock_source.data = (val >> 8) & 3;

  // Reset interrupt and counter
  timer->interrupt = false;
  timer->counter = 0;

  if (timer->wrap_irq) {
    fatal("Wrap IRQ not supported!");
  }

  if ((timer->wrap_irq || timer->target_irq) && !timer->repeat_irq) {
    fatal("One Shot Timer interrupts are not supported!");
  }

  if (timer->negate_irq) {
    fatal("Only pulse interrupts are supported!");
  }

  if (timer->use_sync) {
    fatal("Sync Mode is not supported!");
  }
}

bool needs_gpu(Timer *timer) {
  if (timer->use_sync)
    fatal("Timer Sync Mode Not Supported!");

  ClockType type = clock_type(timer->clock_source, timer->timer_instance);

  return (type == GpuHSync) || (type == GpuPixelClock);
}

Timers init_timers() {
  Timers timers;
  timers.timers[0] = init_timer(Timer0);
  timers.timers[1] = init_timer(Timer1);
  timers.timers[2] = init_timer(Timer2);
  return timers;
}

uint32_t load_timers(Timers *timers, SharedState *shared, Addr offset, AddrType type) {
  if (type == AddrByte)
    fatal("Unexpected Timer Byte Load!");

  Timer *timer = &timers->timers[(offset.data >> 4) & 3];

  timer_sync(timer, shared);

  switch(offset.data & 0xF) {
    case 0:
      return timer->counter;
    case 4:
      return get_timer_mode(timer);
    case 8:
      return timer->target;
    default:
      fatal("Unhandled Timer register");
  }
}

void store_timers(Timers *timers, SharedState *shared, Gpu *gpu, Addr offset, uint32_t val, AddrType type) {
  if (type == AddrByte)
    fatal("Unexpected Timer Byte Load!");

  Timer *timer = &timers->timers[(offset.data >> 4) & 3];

  timer_sync(timer, shared);

  switch(offset.data & 0xF) {
    case 0:
      timer->counter = val;
      break;
    case 4:
      set_timer_mode(timer, val);
      break;
    case 8:
      timer->target = val;
      break;
    default:
      fatal("Unhandled Timer register");
  }

  if (needs_gpu(timer))
    log_error("GPU Timings unimplemented!");

  reconfigure(timer, shared, gpu);
}

void timers_sync(Timers *timers, SharedState *shared) {
  if (needs_sync(&shared->clock, Timer0))
      timer_sync(timers->timers, shared);
  if (needs_sync(&shared->clock, Timer1))
      timer_sync(timers->timers + 1, shared);
  if (needs_sync(&shared->clock, Timer2))
      timer_sync(timers->timers + 2, shared);
}

