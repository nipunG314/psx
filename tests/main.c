#include "unity.h"
#include "cpu.h"

void setUp(void) {
}

void tearDown(void) {
}

void test_branch_delay_slot(void) {
  Cpu cpu = init_cpu("asm_tests/test_branch_delay_slot.bin");

  run_next_ins(&cpu);
  run_next_ins(&cpu);
  run_next_ins(&cpu);

  TEST_ASSERT_EQUAL_UINT32(cpu.regs[0x12], 0x0F000000);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_branch_delay_slot);
  return UNITY_END();
}
