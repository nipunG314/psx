#include <stdbool.h>

#include "types.h"

#ifndef SYNC_H
#define SYNC_H

typedef struct Psx Psx;

typedef enum SyncToken {
  Dma,
  SyncTokenCount
} SyncToken;

typedef struct Synchronizer {
  Cycles last_sync[SyncTokenCount];
  Cycles next_sync[SyncTokenCount];
  Cycles first_event;
} Synchronizer;

Synchronizer init_synchronizer();
Cycles resync(Psx *psx, SyncToken token);
bool is_event_pending(Psx *psx);
void handle_events(Psx *psx);
void set_next_event(Psx *psx, SyncToken token, Cycles delay);
void rebase_counters(Psx *psx);

#endif
