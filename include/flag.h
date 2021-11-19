#include <stdint.h>

#ifndef FLAGS_H
#define FLAGS_H

typedef uint64_t FlagSet;

typedef enum Flag {
  PRINT_PC = 1 << 0,
  PRINT_INS = 1 << 1,
} Flag;

FlagSet flag_set;

static inline int get_flag(Flag flag) {
  return flag_set & flag;
}

static inline void set_flag(Flag flag) {
  flag_set = (flag_set | flag);
}

#endif
