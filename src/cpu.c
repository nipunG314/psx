#include "cpu.h"

Cpu init_cpu() {
  Cpu cpu = {0};
  
  cpu.pc = 0xbfc00000;

  return cpu;
}
