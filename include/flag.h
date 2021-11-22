#include <stdint.h>

#include "log.h"

#ifndef FLAGS_H
#define FLAGS_H

typedef uint64_t FlagSet;

typedef enum Flag {
  PRINT_PC = 1 << 0,
  PRINT_INS = 1 << 1,
  OUTPUT_LOG = 1 << 2
} Flag;

FlagSet flag_set;
uint32_t current_pc;
uint8_t logging_pc;

static inline int get_flag(Flag flag) {
  return flag_set & flag;
}

static inline void set_flag(Flag flag) {
  flag_set = (flag_set | flag);
}

static inline void set_pc(uint32_t pc) {
  current_pc = pc;
}

#define LOG_PC() \
  if (get_flag(PRINT_PC)) \
    log_trace("PC: 0x%08X", current_pc)

#define START_LOGGING_PC() \
  if (get_flag(PRINT_PC)) \
    logging_pc = 1

#define STOP_LOGGING_PC() \
  if (get_flag(PRINT_PC)) \
    logging_pc = 0

#endif
