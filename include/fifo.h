#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

#ifndef GPU_FIFO
#define GPU_FIFO

#define HARD_COMMAND_FIFO_DEPTH 0x10
#define SOFT_COMMAND_FIFO_DEPTH 0x20

typedef struct CommandFifo {
  uint32_t buffer[SOFT_COMMAND_FIFO_DEPTH];
  uint8_t read_index;
  uint8_t write_index;
} CommandFifo;

static inline CommandFifo init_command_fifo() {
  CommandFifo fifo;

  memset(fifo.buffer, 0, SOFT_COMMAND_FIFO_DEPTH * sizeof(uint32_t));
  fifo.read_index = fifo.write_index = 0;

  return fifo;
}

static inline uint8_t command_fifo_len(CommandFifo *fifo) {
  return fifo->write_index - fifo->read_index;
}

static inline bool command_fifo_empty(CommandFifo *fifo) {
  return command_fifo_len(fifo) == 0;
}

static inline bool command_fifo_full(CommandFifo *fifo) {
  return command_fifo_len(fifo) == SOFT_COMMAND_FIFO_DEPTH;
}

static inline void command_fifo_push(CommandFifo *fifo, uint32_t command) {
  if (command_fifo_full(fifo))
    fatal("Attempting to push to full GP0 fifo. Command: 0x%08X", command);

  fifo->buffer[fifo->write_index % SOFT_COMMAND_FIFO_DEPTH] = command;
  fifo->write_index++;
}

static inline uint32_t command_fifo_pop(CommandFifo *fifo) {
  if (command_fifo_empty(fifo))
    fatal("Attempting to pop from empty GP0 fifo");

  uint8_t read_index = fifo->read_index % SOFT_COMMAND_FIFO_DEPTH;
  fifo->read_index++;
  return fifo->buffer[read_index];
}

static inline uint32_t command_fifo_peek(CommandFifo *fifo) {
  if (command_fifo_empty(fifo))
    fatal("Attempting to peek from empty GP0 fifo");

  return fifo->buffer[fifo->read_index % SOFT_COMMAND_FIFO_DEPTH];
}

static inline void command_fifo_clear(CommandFifo *fifo) {
  fifo->read_index = fifo->write_index;
}

#endif
