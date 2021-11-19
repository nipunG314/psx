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

void test_load_delay_slot1(void) {
  Cpu cpu = init_cpu("asm_tests/test_load_delay_slot1.bin");

  run_next_ins(&cpu);
  run_next_ins(&cpu);

  uint32_t prev_value = cpu.regs[0x1];

  run_next_ins(&cpu);
  run_next_ins(&cpu);
  run_next_ins(&cpu);

  TEST_ASSERT_EQUAL_UINT32(prev_value, cpu.regs[0x2]);
  TEST_ASSERT_EQUAL_UINT32(cpu.regs[0x1], cpu.regs[0x3]);
}

void test_load_delay_slot2(void) {
  Cpu cpu = init_cpu("asm_tests/test_load_delay_slot2.bin");

  run_next_ins(&cpu);
  run_next_ins(&cpu);
  run_next_ins(&cpu);
  run_next_ins(&cpu);

  TEST_ASSERT_EQUAL_UINT32(0x2A, cpu.regs[0x1]);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_branch_delay_slot);
  RUN_TEST(test_load_delay_slot1);
  RUN_TEST(test_load_delay_slot2);
  return UNITY_END();
}
