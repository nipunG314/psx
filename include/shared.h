#include "clock.h"

#ifndef SHARED_H
#define SHARED_H

typedef struct SharedState {
  Clock clock;
} SharedState;

static inline SharedState init_shared() {
  SharedState shared;

  shared.clock = init_clock();

  return shared;
}

#endif
