#include <stdlib.h>
#include <string.h>

#include "output_logger.h"

OutputLogBuffer *buffers = NULL;

size_t const init_output_log() {
  if (!get_flag(OUTPUT_LOG))
    return 0;
  if (next_pool_index >= OUTPUT_LOG_POOL_SIZE)
    return 0;

  if (buffers == NULL)
    buffers = calloc(OUTPUT_LOG_POOL_SIZE, sizeof(OutputLogBuffer));

  memset(buffers[next_pool_index].buffer, 0, OUTPUT_LOG_LINE_LEN);
  buffers[next_pool_index].pos = 0;
  next_pool_index++;

  return next_pool_index - 1;
}

void print_output_log(size_t index) {
  if (!get_flag(OUTPUT_LOG))
    return;

  printf("%s\n", buffers[index].buffer);
  memset(buffers[index].buffer, 0, OUTPUT_LOG_LINE_LEN);
  buffers[index].pos = 0;
}
