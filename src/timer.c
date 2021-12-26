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

bool needs_gpu(Timer *timer) {
  if (timer->use_sync)
    fatal("Timer Sync Mode Not Supported!");

  ClockType type = clock_type(timer->clock_source, timer->timer_instance);

  return (type == GpuHSync) || (type == GpuPixelClock);
}

