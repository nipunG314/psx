#include "cpu.h"
#include "interconnect.h"

Cpu init_cpu(char const *bios_filename) {
  Cpu cpu = {0};
  
  cpu.pc = BIOS_START;
  cpu.inter = init_interconnect(bios_filename);

  return cpu;
}
