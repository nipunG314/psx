#include "cpu.h"

int main(void) {
  
  Cpu cpu = init_cpu("SCPH1001.BIN");

  while (1) {
    run_next_ins(&cpu);
  }

  destroy_cpu(&cpu);

  return 0;
}
