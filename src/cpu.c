#include <stdlib.h>

#include "cpu.h"
#include "log.h"
#include "interconnect.h"

Cpu init_cpu(char const *bios_filename) {
  Cpu cpu = {0};
  
  cpu.pc = BIOS_START;
  for(int index = 0; index < 32; index++)
    cpu.regs[index] = 0xDEADDEAD;
  cpu.regs[0] = 0x0;
  cpu.inter = init_interconnect(bios_filename);

  return cpu;
}

uint32_t load_ins(Cpu *cpu, uint32_t addr) {
  return load_inter_ins(&cpu->inter, addr);
}

void set_reg(Cpu *cpu, uint8_t index, uint32_t value) {
  cpu->regs[index] = value;
  cpu->regs[0] = 0x0;
}

void decode_and_execute(Cpu *cpu, uint32_t ins) {
  log_error("DecodeError: Unhandled instruction: 0x%X", ins);
  exit(EXIT_FAILURE);
}

void run_next_ins(Cpu *cpu) {
  uint32_t ins = load_ins(cpu, cpu->pc);

  cpu->pc += 4;

  decode_and_execute(cpu, ins);
}
