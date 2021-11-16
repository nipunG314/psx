#include "test_assembler.h"

int main(void) {
  assemble_test("asm_tests/test_branch_delay_slot.asm", "asm_tests/test_branch_delay_slot.bin");

  return 0;
}
