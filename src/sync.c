#include <string.h>
#include <math.h>

#include "psx.h"
#include "sync.h"

Synchronizer init_synchronizer() {
  Synchronizer sync;

  memset(sync.last_sync, 0, sizeof(Cycles) * SyncTokenCount); 
  memset(sync.next_sync, 0, sizeof(Cycles) * SyncTokenCount); 
  sync.first_event = MAKE_Cycles(0);

  return sync;
}

void refresh_first_event(Synchronizer *sync) {
  int32_t first_event = INT32_MAX;
  
  for(size_t count = 0; count < SyncTokenCount; count++)
    first_event = fmin(first_event, sync->next_sync[count].data);

  sync->first_event = MAKE_Cycles(first_event);
}

Cycles resync(Psx *psx, SyncToken token) {
  int32_t elapsed = psx->cycles_counter.data - psx->sync.last_sync[token].data;

  if (elapsed <= 0)
    return MAKE_Cycles(0);

  psx->sync.last_sync[token] = psx->cycles_counter;

  return MAKE_Cycles(elapsed);
}

bool is_event_pending(Psx *psx) {
  return psx->cycles_counter.data >= psx->sync.first_event.data;
}

void handle_events(Psx *psx) {
  while (is_event_pending(psx)) {
    int32_t delta = psx->cycles_counter.data - psx->sync.first_event.data;
    psx->cycles_counter.data -= delta;
    
    if (psx->sync.first_event.data >= psx->sync.next_sync[Dma].data) {
      // ToDo: Run DMA
    }

    psx->cycles_counter.data += delta;
  }
}

void set_next_event(Psx *psx, SyncToken token, Cycles delay) {
  psx->sync.next_sync[token] = MAKE_Cycles(psx->cycles_counter.data + delay.data);
  refresh_first_event(&psx->sync);
}

void rebase_counters(Psx *psx) {
  for (size_t count = 0; count < SyncTokenCount; count++) {
    psx->sync.last_sync[count].data -= psx->cycles_counter.data;
    psx->sync.next_sync[count].data -= psx->cycles_counter.data;
  }
  psx->sync.first_event.data -= psx->cycles_counter.data;

  psx->cycles_counter = MAKE_Cycles(0);
}
