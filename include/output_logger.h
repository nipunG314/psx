#include <stdio.h>

#include "flag.h"

#ifndef OUTPUT_LOGGER_H
#define OUTPUT_LOGGER_H

#define OUTPUT_LOG_LINE_LEN 1000
#define OUTPUT_LOG_POOL_SIZE 10

typedef struct OutputLogBuffer {
  char buffer[OUTPUT_LOG_LINE_LEN];
  size_t pos;
} OutputLogBuffer;

extern OutputLogBuffer *buffers;
static size_t next_pool_index;

size_t const init_output_log();
void print_output_log(size_t index);

#define LOG_OUTPUT(index, fmt, ...) \
  if (get_flag(OUTPUT_LOG)) \
    buffers[index].pos = sprintf(buffers[index].buffer + buffers[index].pos, fmt, __VA_ARGS__) 

#endif
