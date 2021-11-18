#include <stdio.h>
#include <string.h>

#include "cpu.h"
#include "flag.h"

FlagSet flag_set = 0x0;

void set_env(int argc, char **argv) {
  for(size_t i=1; i<argc; i++) {
    if (strcmp(argv[i], "--print-pc") == 0)
      set_flag(PRINT_PC);
    else if (strcmp(argv[i], "--print-ins") == 0)
      set_flag(PRINT_INS);
  }
}

int main(int argc, char **argv) {
  set_env(argc, argv);

  Cpu cpu = init_cpu("SCPH1001.BIN");

  while (1) {
    run_next_ins(&cpu);
  }

  destroy_cpu(&cpu);

  return 0;
}
