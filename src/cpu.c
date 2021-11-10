#include "cpu.h"
#include "interconnect.h"

Cpu init_cpu() {
  Cpu cpu = {0};
  
  cpu.pc = BIOS_START;

  return cpu;
}
