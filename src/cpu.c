#include <stdlib.h>
#include <string.h>

#include "psx.h"
#include "cpu.h"
#include "log.h"

Cpu init_cpu() {
  Cpu cpu;

  cpu.pc = MAKE_Addr(0xBFC00000);
  cpu.next_pc = MAKE_Addr(cpu.pc.data + 4);
  cpu.current_pc = MAKE_Addr(0);
  for(int index = 0; index < 32; index++) {
    cpu.regs[index] = 0xDEADDEAD;
  }
  cpu.regs[0] = 0x0;
  cpu.hi = cpu.lo = 0xDEADDEAD;
  cpu.load = MAKE_Load(false, MAKE_RegIndex(0), 0, MAKE_Cycles(0));
  cpu.load_cycles_index = MAKE_RegIndex(0);
  cpu.branch = false;
  cpu.delay_slot = false;
  cpu.mult_div_end = MAKE_Cycles(0);
  cpu.optable_offset = 0;

  return cpu;
}

void set_reg(Cpu *cpu, RegIndex index, uint32_t value) {
  cpu->regs[index.data] = value;
  cpu->regs[0] = 0x0;
}

RegIndex reg_dep(Cpu *cpu, RegIndex index) {
  uint8_t c0 = cpu->load_cycles[0];  
  cpu->load_cycles[index.data] = 0;
  cpu->load_cycles[0] = c0;

  return index;
}

void branch(Cpu *cpu, uint32_t offset) {
  offset = offset << 2;
  cpu->next_pc = MAKE_Addr(cpu->pc.data + offset);
  cpu->branch = true;
}

void delayed_load(Cpu *cpu) {
  if (cpu->load.active) {
    set_reg(cpu, cpu->load.index, cpu->load.val);

    // Update Load Cycles
    cpu->load_cycles[cpu->load.index.data] = cpu->load.cycle_count.data;
    cpu->load_cycles_index = cpu->load.index;

    cpu->load.active = false;
  }
}

void delayed_load_chain(Cpu *cpu, RegIndex index, uint32_t val, Cycles cycle_count, bool sync) {
  if (cpu->load.active && cpu->load.index.data != index.data) {
    set_reg(cpu, cpu->load.index, cpu->load.val);

    if (!sync) {
      cpu->load_cycles[cpu->load.index.data] = cpu->load.cycle_count.data;
      cpu->load_cycles_index = cpu->load.index;
    }
  }

  cpu->load.active = true;
  cpu->load.index = index;
  cpu->load.val = val;
  cpu->load.cycle_count = cycle_count;
}

void load_sync(Cpu *cpu) {
  cpu->load_cycles[cpu->load_cycles_index.data] = 0;
}

void handle_irq_changed(Psx *psx) {
  psx->cpu.optable_offset = (irq_pending(psx)) ? 64 : 0;
}

void cpu_rebase_counters(Cpu *cpu, Cycles count) {
  cpu->mult_div_end.data -= count.data;
}

void exception(Psx *psx, Exception exp) {
  Addr handler_addr = enter_exception(psx, exp); 

  psx->cpu.pc = handler_addr;
  psx->cpu.next_pc = MAKE_Addr(psx->cpu.pc.data + 4);
}

void instruction_tick(Psx *psx) {
  uint32_t index = psx->cpu.load_cycles_index.data;

  if (psx->cpu.load_cycles[index] > 0)
    psx->cpu.load_cycles[index]--;
  else
    tick(psx, MAKE_Cycles(1));
}

LoadAttribute cpu_load(Psx *psx, Addr addr, AddrType type, bool lwc) {
  load_sync(&psx->cpu);

  {
    addr = mask_region(addr);

    int32_t offset = range_contains(range(SCRATCH_PAD), addr);
    if (offset >= 0) {
      log_error("Unhandled read from SCRATCH_PAD. Addr: 0x%08X, Type: 0x%08X", addr, type);
      return MAKE_LoadAttribute(0, 0);
    }
  }

  if (!psx->cpu.load.active)
    tick(psx, MAKE_Cycles(2));

  Cycles cc_old = psx->cycles_counter;
  uint32_t val = load(psx, addr, type);

  tick(psx, MAKE_Cycles(lwc ? 1 : 2)); 

  return MAKE_LoadAttribute(val, psx->cycles_counter.data - cc_old.data);
}

void cpu_store(Psx *psx, Addr addr, uint32_t val, AddrType type) {
  if (cache_isolated(&psx->cop0))
    return;

  store(psx, addr, val, type); 
}

void sync_mult_div(Psx *psx) {
  int32_t block_duration = psx->cpu.mult_div_end.data - psx->cycles_counter.data;

  if (block_duration == 1)
    return;

  if (block_duration > 0) {
    psx->cycles_counter = psx->cpu.mult_div_end;

    RegIndex index = psx->cpu.load_cycles_index;
    if (psx->cpu.load_cycles[index.data] <= block_duration)
      psx->cpu.load_cycles[index.data] = 0;
    else
      psx->cpu.load_cycles[index.data] -= block_duration;
  }
}

void op_lui(Psx *psx, Ins ins) {
  uint32_t imm = get_imm(ins);
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rt, imm << 16);
}

void op_ori(Psx *psx, Ins ins) {
  uint32_t imm = get_imm(ins);
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  uint32_t reg_s = psx->cpu.regs[rs.data];

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rt, imm | reg_s);
}

void op_xori(Psx *psx, Ins ins) {
  uint32_t imm = get_imm(ins);
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  uint32_t reg_s = psx->cpu.regs[rs.data];

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rt, imm ^ reg_s);
}

void op_andi(Psx *psx, Ins ins) {
  uint32_t imm = get_imm(ins);
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  uint32_t reg_s = psx->cpu.regs[rs.data];

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rt, imm & reg_s);
}

void op_sw(Psx *psx, Ins ins) {
  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  Addr addr = MAKE_Addr(imm_se + psx->cpu.regs[rs.data]);
  uint32_t reg_t = psx->cpu.regs[rt.data];

  delayed_load(&psx->cpu);

  if (addr.data % 4) {
    exception(psx, StoreAddressError);
    return;
  }

  cpu_store(psx, addr, reg_t, AddrWord);
}

void op_sll(Psx *psx, Ins ins) {
  uint32_t shift = get_shift(ins);
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  RegIndex rd = reg_dep(&psx->cpu, get_rd(ins));
  uint32_t reg_t = psx->cpu.regs[rt.data];

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rd, reg_t << shift);
}

void op_sllv(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  RegIndex rd = reg_dep(&psx->cpu, get_rd(ins));
  uint32_t reg_s = psx->cpu.regs[rs.data];
  uint32_t reg_t = psx->cpu.regs[rt.data];

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rd, reg_t << (reg_s & 0x1F));
}

void op_addiu(Psx *psx, Ins ins) {
  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  uint32_t reg_s = psx->cpu.regs[rs.data];

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rt, reg_s + imm_se);
}

void op_addi(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  uint32_t imm_se = get_imm_se(ins);
  uint32_t res = imm_se + psx->cpu.regs[rs.data];
  bool overflow = ((psx->cpu.regs[rs.data] ^ res) & (imm_se ^ res)) >> 31;

  delayed_load(&psx->cpu);

  if (overflow) {
    exception(psx, Overflow);
    return;
  }

  set_reg(&psx->cpu, rt, res);
}

void op_j(Psx *psx, Ins ins) {
  uint32_t imm_jump = get_imm_jump(ins);
  psx->cpu.branch = true;

  psx->cpu.next_pc = MAKE_Addr((psx->cpu.next_pc.data & 0xF0000000) | (imm_jump << 2));

  delayed_load(&psx->cpu);
}

void op_jal(Psx *psx, Ins ins) {
  uint32_t ra = psx->cpu.next_pc.data;
  op_j(psx, ins);
  set_reg(&psx->cpu, MAKE_RegIndex(0x1F), ra);
}

void op_jalr(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rd = reg_dep(&psx->cpu, get_rd(ins));

  uint32_t ra = psx->cpu.next_pc.data;
  psx->cpu.next_pc = MAKE_Addr(psx->cpu.regs[rs.data]);

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rd, ra);
  psx->cpu.branch = true;
}

void op_or(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  RegIndex rd = reg_dep(&psx->cpu, get_rd(ins));
  uint32_t reg_s = psx->cpu.regs[rs.data];
  uint32_t reg_t = psx->cpu.regs[rt.data];

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rd, reg_s | reg_t);
}

void op_and(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  RegIndex rd = reg_dep(&psx->cpu, get_rd(ins));
  uint32_t reg_s = psx->cpu.regs[rs.data];
  uint32_t reg_t = psx->cpu.regs[rt.data];

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rd, reg_s & reg_t);
}

void op_nor(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  RegIndex rd = reg_dep(&psx->cpu, get_rd(ins));
  uint32_t reg_s = psx->cpu.regs[rs.data];
  uint32_t reg_t = psx->cpu.regs[rt.data];

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rd, ~(reg_s | reg_t));
}

void op_xor(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  RegIndex rd = reg_dep(&psx->cpu, get_rd(ins));
  uint32_t reg_s = psx->cpu.regs[rs.data];
  uint32_t reg_t = psx->cpu.regs[rt.data];

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rd, reg_s ^ reg_t);
}

void op_mtc0(Psx *psx, Ins ins) {
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  RegIndex cop_reg = MAKE_RegIndex(get_cop_reg(ins));
  uint32_t val = psx->cpu.regs[rt.data];

  delayed_load(&psx->cpu);

  mtc0(psx, cop_reg, val);
}

void op_mfc0(Psx *psx, Ins ins) {
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  uint8_t cop_reg = get_cop_reg(ins);
  uint32_t val = mfc0(psx, MAKE_RegIndex(cop_reg));

  delayed_load_chain(&psx->cpu, rt, val, MAKE_Cycles(0), false);
}

void op_lwc0(Psx *psx, Ins ins) {
  delayed_load(&psx->cpu);

  log_error("LWC0 unavailable!");

  exception(psx, CoprocessorError);
}

void op_lwc1(Psx *psx, Ins ins) {
  delayed_load(&psx->cpu);

  log_error("LWC1 unavailable!");

  exception(psx, CoprocessorError);
}

void op_lwc2(Psx *psx, Ins ins) {
  fatal("Unhandled GTE LWC. Ins: 0x%08X", ins);
}

void op_lwc3(Psx *psx, Ins ins) {
  delayed_load(&psx->cpu);

  log_error("LWC3 unavailable!");

  exception(psx, CoprocessorError);
}

void op_swc0(Psx *psx, Ins ins) {
  delayed_load(&psx->cpu);

  log_error("SWC0 unavailable!");

  exception(psx, CoprocessorError);
}

void op_swc1(Psx *psx, Ins ins) {
  delayed_load(&psx->cpu);

  log_error("SWC1 unavailable!");

  exception(psx, CoprocessorError);
}

void op_swc2(Psx *psx, Ins ins) {
  fatal("Unhandled GTE SWC. Ins: 0x%08X", ins);
}

void op_swc3(Psx *psx, Ins ins) {
  delayed_load(&psx->cpu);

  log_error("SWC3 unavailable!");

  exception(psx, CoprocessorError);
}

void op_rfe(Psx *psx, Ins ins) {
  delayed_load(&psx->cpu);

  if ((ins.data & 0x3F) != 0x10)
    fatal("Invalid cop0 instruction: 0x%08X", ins);

  return_from_exception(psx);
}

void op_beq(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  uint32_t imm_se = get_imm_se(ins);

  if (psx->cpu.regs[rs.data] == psx->cpu.regs[rt.data])
    branch(&psx->cpu, imm_se);

  delayed_load(&psx->cpu);
}

void op_bne(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  uint32_t imm_se = get_imm_se(ins);

  if (psx->cpu.regs[rs.data] != psx->cpu.regs[rt.data])
    branch(&psx->cpu, imm_se);

  delayed_load(&psx->cpu);
}

void op_blez(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  uint32_t imm_se = get_imm_se(ins);

  int32_t reg_s = psx->cpu.regs[rs.data];

  if (reg_s <= 0)
    branch(&psx->cpu, imm_se);

  delayed_load(&psx->cpu);
}

void op_bgtz(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  uint32_t imm_se = get_imm_se(ins);

  int32_t reg_s = psx->cpu.regs[rs.data];

  if (reg_s > 0)
    branch(&psx->cpu, imm_se);

  delayed_load(&psx->cpu);
}

void op_lw(Psx *psx, Ins ins) {
  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));

  Addr addr = MAKE_Addr(psx->cpu.regs[rs.data] + imm_se);

  if (addr.data % 4) {
    delayed_load(&psx->cpu);
    exception(psx, LoadAddressError);
    return;
  }

  LoadAttribute data = cpu_load(psx, addr, AddrWord, false);

  delayed_load_chain(&psx->cpu, rt, data.val, MAKE_Cycles(data.cycle_count), true);
}

void op_lb(Psx *psx, Ins ins) {
  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));

  Addr addr = MAKE_Addr(psx->cpu.regs[rs.data] + imm_se);
  LoadAttribute data = cpu_load(psx, addr, AddrByte, false);
  int8_t value_data = data.val;

  delayed_load_chain(&psx->cpu, rt, value_data, MAKE_Cycles(data.cycle_count), true);
}

void op_lbu(Psx *psx, Ins ins) {
  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));

  Addr addr = MAKE_Addr(psx->cpu.regs[rs.data] + imm_se);
  LoadAttribute data = cpu_load(psx, addr, AddrByte, false);

  delayed_load_chain(&psx->cpu, rt, data.val, MAKE_Cycles(data.cycle_count), true);
}

void op_sltu(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  RegIndex rd = reg_dep(&psx->cpu, get_rd(ins));
  uint32_t reg_s = psx->cpu.regs[rs.data];
  uint32_t reg_t = psx->cpu.regs[rt.data];

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rd, reg_s < reg_t);
}

void op_slt(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  RegIndex rd = reg_dep(&psx->cpu, get_rd(ins));
  int32_t reg_s = psx->cpu.regs[rs.data];
  int32_t reg_t = psx->cpu.regs[rt.data];

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rd, reg_s < reg_t);
}

void op_slti(Psx *psx, Ins ins) {
  int32_t imm_se = get_imm_se(ins);
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  int32_t reg_s = psx->cpu.regs[rs.data];

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rt, reg_s < imm_se);
}

void op_sltiu(Psx *psx, Ins ins) {
  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  uint32_t reg_s = psx->cpu.regs[rs.data];

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rt, reg_s < imm_se);
}

void op_add(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  RegIndex rd = reg_dep(&psx->cpu, get_rd(ins));
  uint32_t res = psx->cpu.regs[rs.data] + psx->cpu.regs[rt.data];
  bool overflow = ((psx->cpu.regs[rs.data] ^ res) & (psx->cpu.regs[rt.data] ^ res)) >> 31;

  delayed_load(&psx->cpu);

  if (overflow) {
    exception(psx, Overflow);
    return;
  }

  set_reg(&psx->cpu, rd, res);
}

void op_addu(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  RegIndex rd = reg_dep(&psx->cpu, get_rd(ins));
  uint32_t reg_s = psx->cpu.regs[rs.data];
  uint32_t reg_t = psx->cpu.regs[rt.data];

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rd, reg_s + reg_t);
}

void op_sh(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  uint32_t imm_se = get_imm_se(ins);
  Addr addr = MAKE_Addr(psx->cpu.regs[rs.data] + imm_se);
  uint32_t reg_t = psx->cpu.regs[rt.data];

  delayed_load(&psx->cpu);

  if (addr.data % 2) {
    exception(psx, StoreAddressError);
    return;
  }

  cpu_store(psx, addr, reg_t, AddrHalf);
}

void op_sb(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  uint32_t imm_se = get_imm_se(ins);

  Addr addr = MAKE_Addr(psx->cpu.regs[rs.data] + imm_se);
  uint32_t reg_t = psx->cpu.regs[rt.data];

  delayed_load(&psx->cpu);

  store(psx, addr, reg_t, AddrByte);
}

void op_jr(Psx *psx, Ins ins) {
  psx->cpu.next_pc = MAKE_Addr(psx->cpu.regs[reg_dep(&psx->cpu, get_rs(ins)).data]);
  delayed_load(&psx->cpu);
  psx->cpu.branch = true;
}

void op_bxx(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  uint32_t imm_se = get_imm_se(ins);

  int is_bgez = (ins.data >> 16) & 0x1 ? 1 : 0;
  int is_link = (((ins.data >> 17) & 0xf) == 0x8) ? 1 : 0;

  int32_t reg_s = psx->cpu.regs[rs.data];
  uint32_t test = (reg_s < 0);
  test = test ^ is_bgez;

  delayed_load(&psx->cpu);

  if (is_link)
    set_reg(&psx->cpu, MAKE_RegIndex(0x1F), psx->cpu.next_pc.data);

  if (test != 0) {
    branch(&psx->cpu, imm_se);
  }
}

void op_sub(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  RegIndex rd = reg_dep(&psx->cpu, get_rd(ins));
  uint32_t res = psx->cpu.regs[rs.data] - psx->cpu.regs[rt.data];
  bool overflow = ((psx->cpu.regs[rs.data] ^ res) & ~(psx->cpu.regs[rt.data] ^ res)) >> 31;

  delayed_load(&psx->cpu);

  if (overflow) {
    exception(psx, Overflow);
    return;
  }

  set_reg(&psx->cpu, rd, res);
}

void op_subu(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  RegIndex rd = reg_dep(&psx->cpu, get_rd(ins));
  uint32_t reg_s = psx->cpu.regs[rs.data];
  uint32_t reg_t = psx->cpu.regs[rt.data];

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rd, reg_s - reg_t);
}

void op_sra(Psx *psx, Ins ins) {
  uint32_t shift = get_shift(ins);
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  RegIndex rd = reg_dep(&psx->cpu, get_rd(ins));

  int32_t reg_t = psx->cpu.regs[rt.data];
  reg_t = reg_t >> shift;

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rd, reg_t);
}

void op_srav(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  RegIndex rd = reg_dep(&psx->cpu, get_rd(ins));

  int32_t reg_t = psx->cpu.regs[rt.data];
  uint8_t shift = psx->cpu.regs[rs.data] & 0x1F;
  reg_t = reg_t >> shift;

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rd, reg_t);
}

void op_srl(Psx *psx, Ins ins) {
  uint32_t shift = get_shift(ins);
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  RegIndex rd = reg_dep(&psx->cpu, get_rd(ins));
  uint32_t reg_t = psx->cpu.regs[rt.data];

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rd, reg_t >> shift);
}

void op_srlv(Psx *psx, Ins ins) {
  RegIndex rs =reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  RegIndex rd = reg_dep(&psx->cpu, get_rd(ins));

  uint32_t shift = psx->cpu.regs[rs.data] & 0x1F;
  uint32_t reg_t = psx->cpu.regs[rt.data];

  delayed_load(&psx->cpu);

  set_reg(&psx->cpu, rd, reg_t >> shift);
}

void op_lh(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  uint32_t imm_se = get_imm_se(ins);
  Addr addr = MAKE_Addr(psx->cpu.regs[rs.data] + imm_se);

  if (addr.data % 2) {
    delayed_load(&psx->cpu);
    exception(psx, LoadAddressError);
    return;
  }

  LoadAttribute data = cpu_load(psx, addr, AddrHalf, false);
  int16_t value_data = data.val;

  delayed_load_chain(&psx->cpu, rt, value_data, MAKE_Cycles(data.cycle_count), true);
}

void op_lhu(Psx *psx, Ins ins) {
  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));

  Addr addr = MAKE_Addr(psx->cpu.regs[rs.data] + imm_se);

  if (addr.data % 2) {
    delayed_load(&psx->cpu);
    exception(psx, LoadAddressError);
    return;
  }

  LoadAttribute data = cpu_load(psx, addr, AddrHalf, false);

  delayed_load_chain(&psx->cpu, rt, data.val, MAKE_Cycles(data.cycle_count), true);
}

void op_lwl(Psx *psx, Ins ins) {
  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));

  Addr addr = MAKE_Addr(psx->cpu.regs[rs.data] + imm_se);
  uint32_t cur_val;
  if (psx->cpu.load.active && psx->cpu.load.index.data == rt.data)
    cur_val = psx->cpu.load.val;
  else
    cur_val = psx->cpu.regs[rt.data];

  Addr aligned_addr = MAKE_Addr(addr.data & ~3);
  LoadAttribute aligned_load = cpu_load(psx, aligned_addr, AddrWord, false);
  Ins aligned_ins = MAKE_Ins(aligned_load.val);
  uint32_t val;

  switch (addr.data & 3) {
    case 0:
      val = (cur_val & 0x00FFFFFF) | (aligned_ins.data << 24);
      break;
    case 1:
      val = (cur_val & 0x0000FFFF) | (aligned_ins.data << 16);
      break;
    case 2:
      val = (cur_val & 0x000000FF) | (aligned_ins.data << 8);
      break;
    case 3:
      val = (cur_val & 0x00000000) | (aligned_ins.data << 0);
      break;
    default:
      fatal("Unreachable Statement!");
  }

  delayed_load_chain(&psx->cpu, rt, val, MAKE_Cycles(aligned_load.cycle_count), true);
}

void op_lwr(Psx *psx, Ins ins) {
  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));

  Addr addr = MAKE_Addr(psx->cpu.regs[rs.data] + imm_se);
  uint32_t cur_val;
  if (psx->cpu.load.active && psx->cpu.load.index.data == rt.data)
    cur_val = psx->cpu.load.val;
  else
    cur_val = psx->cpu.regs[rt.data];

  Addr aligned_addr = MAKE_Addr(addr.data & ~3);
  LoadAttribute aligned_load = cpu_load(psx, aligned_addr, AddrWord, false);
  Ins aligned_ins = MAKE_Ins(aligned_load.val);

  uint32_t val;
  switch (addr.data & 3) {
    case 0:
      val = (cur_val & 0x00000000) | (aligned_ins.data >> 0);
      break;
    case 1:
      val = (cur_val & 0xFF000000) | (aligned_ins.data >> 8);
      break;
    case 2:
      val = (cur_val & 0xFFFF0000) | (aligned_ins.data >> 16);
      break;
    case 3:
      val = (cur_val & 0xFFFFFF00) | (aligned_ins.data >> 24);
      break;
    default:
      fatal("Unreachable Statement!");
  }

  delayed_load_chain(&psx->cpu, rt, val, MAKE_Cycles(aligned_load.cycle_count), true);
}

void op_swl(Psx *psx, Ins ins) {
  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));

  Addr addr = MAKE_Addr(psx->cpu.regs[rs.data] + imm_se);
  uint32_t val = psx->cpu.regs[rt.data];

  Addr aligned_addr = MAKE_Addr(addr.data & ~3);
  uint32_t cur_val = cpu_load(psx, aligned_addr, AddrWord, false).val;
  uint32_t final_val;
  switch (addr.data & 3) {
    case 0:
      final_val = (cur_val & 0xFFFFFF00) | (val >> 24);
      break;
    case 1:
      final_val = (cur_val & 0xFFFF0000) | (val >> 16);
      break;
    case 2:
      final_val = (cur_val & 0xFF000000) | (val >> 8);
      break;
    case 3:
      final_val = (cur_val & 0x00000000) | (val >> 0);
      break;
    default:
      fatal("Unreachable Statement!");
  }

  delayed_load(&psx->cpu);

  cpu_store(psx, aligned_addr, final_val, AddrWord);
}

void op_swr(Psx *psx, Ins ins) {
  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));

  Addr addr = MAKE_Addr(psx->cpu.regs[rs.data] + imm_se);
  uint32_t val = psx->cpu.regs[rt.data];

  Addr aligned_addr = MAKE_Addr(addr.data & ~3);
  uint32_t cur_val = cpu_load(psx, aligned_addr, AddrWord, false).val;

  uint32_t final_val;
  switch (addr.data & 3) {
    case 0:
      final_val = (cur_val & 0x00000000) | (val << 0);
      break;
    case 1:
      final_val = (cur_val & 0x000000FF) | (val << 8);
      break;
    case 2:
      final_val = (cur_val & 0x0000FFFF) | (val << 16);
      break;
    case 3:
      final_val = (cur_val & 0x00FFFFFF) | (val << 24);
      break;
    default:
      fatal("Unreachable Statement!");
  }

  delayed_load(&psx->cpu);

  cpu_store(psx, aligned_addr, final_val, AddrWord);
}
void op_mult(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  int32_t reg_s = psx->cpu.regs[rs.data];
  int32_t reg_t = psx->cpu.regs[rt.data];
  int64_t reg_s_64 = reg_s;
  int64_t reg_t_64 = reg_t;

  delayed_load(&psx->cpu);

  uint64_t prod = reg_s_64 * reg_t_64;
  psx->cpu.hi = prod >> 32;
  psx->cpu.lo = prod;
}

void op_multu(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  uint64_t reg_s = psx->cpu.regs[rs.data];
  uint64_t reg_t = psx->cpu.regs[rt.data];

  delayed_load(&psx->cpu);

  uint64_t prod = reg_s * reg_t;
  psx->cpu.hi = prod >> 32;
  psx->cpu.lo = prod;
}

void op_div(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  int32_t reg_s = psx->cpu.regs[rs.data];
  int32_t reg_t = psx->cpu.regs[rt.data];

  delayed_load(&psx->cpu);

  if (reg_t == 0) {
    psx->cpu.hi = reg_s;
    psx->cpu.lo = (reg_s >= 0) ? 0xFFFFFFFF : 0x1;
  } else if (reg_s == 0x80000000 && reg_t == 0xFFFFFFFF) {
    psx->cpu.hi = 0x0;
    psx->cpu.lo = reg_s;
  } else {
    psx->cpu.hi = reg_s % reg_t;
    psx->cpu.lo = reg_s / reg_t;
  }
}

void op_divu(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));
  RegIndex rt = reg_dep(&psx->cpu, get_rt(ins));
  uint32_t reg_s = psx->cpu.regs[rs.data];
  uint32_t reg_t = psx->cpu.regs[rt.data];

  delayed_load(&psx->cpu);

  if (reg_t == 0) {
    psx->cpu.hi = reg_s;
    psx->cpu.lo = 0xFFFFFFFF;
  } else {
    psx->cpu.hi = reg_s % reg_t;
    psx->cpu.lo = reg_s / reg_t;
  }
}

void op_mflo(Psx *psx, Ins ins) {
  RegIndex rd = reg_dep(&psx->cpu, get_rd(ins));

  delayed_load(&psx->cpu);

  sync_mult_div(psx);

  set_reg(&psx->cpu, rd, psx->cpu.lo);
}

void op_mtlo(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));

  psx->cpu.lo = psx->cpu.regs[rs.data];

  delayed_load(&psx->cpu);
}

void op_mfhi(Psx *psx, Ins ins) {
  RegIndex rd = reg_dep(&psx->cpu, get_rd(ins));

  delayed_load(&psx->cpu);

  sync_mult_div(psx);

  set_reg(&psx->cpu, rd, psx->cpu.hi);
}

void op_mthi(Psx *psx, Ins ins) {
  RegIndex rs = reg_dep(&psx->cpu, get_rs(ins));

  psx->cpu.hi = psx->cpu.regs[rs.data];

  delayed_load(&psx->cpu);
}

void op_syscall(Psx *psx, Ins ins) {
  delayed_load(&psx->cpu);

  exception(psx, SysCall);
}

void op_break(Psx *psx, Ins ins) {
  delayed_load(&psx->cpu);

  exception(psx, Break);
}

void op_mfc2(Psx *psx, Ins ins) {
  log_error("STUB: op_mfc2 unimplemented!");
}

void op_cfc2(Psx *psx, Ins ins) {
  log_error("STUB: op_cfc2 unimplemented!");
}

void op_mtc2(Psx *psx, Ins ins) {
  log_error("STUB: op_mtc2 unimplemented!");
}

void op_ctc2(Psx *psx, Ins ins) {
  log_error("STUB: op_ctc2 unimplemented!");
}

void op_cop0(Psx *psx, Ins ins) {
  switch (get_cop_func(ins)) {
    case 0x0:
      op_mfc0(psx, ins);
      break;
    case 0x4:
      op_mtc0(psx, ins);
      break;
    case 0x10:
      op_rfe(psx, ins);
      break;
    default:
      exception(psx, IllegalInstruction);
  }
}

void op_cop1(Psx *psx, Ins ins) {
  delayed_load(&psx->cpu);

  log_error("Cop1 not available!");

  exception(psx, CoprocessorError);
}

void op_cop2(Psx *psx, Ins ins) {
  // ToDo: Check if GTE is enabled in cop0's
  // status register. Futhermore, we need to
  // wait two cycles after raising the flag in
  // the status register before GTE can be accessed.
  uint32_t opcode = get_cop_func(ins);

  if (opcode & 0x10) {
    fatal("GTE Command Unhandled! Ins: 0x%08X, Opcode: 0x%08X", ins, opcode);
  } else {
    switch(opcode) {
      case 0x0:
        op_mfc2(psx, ins);
        break;
      case 0x2:
        op_cfc2(psx, ins);
        break;
      case 0x4:
        op_mtc2(psx, ins);
        break;
      case 0x6:
        op_ctc2(psx, ins);
        break;
      default:
        exception(psx, IllegalInstruction);
    }
  }
}

void op_cop3(Psx *psx, Ins ins) {
  delayed_load(&psx->cpu);

  log_error("Cop3 not available!");

  exception(psx, CoprocessorError);
}

void op_irq(Psx *psx, Ins ins) {
  delayed_load(&psx->cpu);

  exception(psx, Interrupt);
}

void op_illegal(Psx *psx, Ins ins) {
  delayed_load(&psx->cpu);

  log_error("Illegal Instruction. Ins: 0x%08X, PC: 0x%08X", ins.data, psx->cpu.current_pc.data);

  exception(psx, IllegalInstruction);
}

OpFunc op_function_handlers[] = {
  // 0x00
  op_sll,      op_illegal,  op_srl,      op_sra,
  op_sllv,     op_illegal,  op_srlv,     op_srav,
  op_jr,       op_jalr,     op_illegal,  op_illegal,
  op_syscall,  op_break,    op_illegal,  op_illegal,
  // 0x10
  op_mfhi,     op_mthi,     op_mflo,     op_mtlo,
  op_illegal,  op_illegal,  op_illegal,  op_illegal,
  op_mult,     op_multu,    op_div,      op_divu,
  op_illegal,  op_illegal,  op_illegal,  op_illegal,
  // 0x20
  op_add,      op_addu,     op_sub,      op_subu,
  op_and,      op_or,       op_xor,      op_nor,
  op_illegal,  op_illegal,  op_slt,      op_sltu,
  op_illegal,  op_illegal,  op_illegal,  op_illegal,
  // 0x30
  op_illegal,  op_illegal,  op_illegal,  op_illegal,
  op_illegal,  op_illegal,  op_illegal,  op_illegal,
  op_illegal,  op_illegal,  op_illegal,  op_illegal,
  op_illegal,  op_illegal,  op_illegal,  op_illegal,
};

void op_function(Psx *psx, Ins ins) {
  op_function_handlers[get_sub_func(ins)](psx, ins);
}

OpFunc op_handlers[] = {
  // 0x00
  op_function, op_bxx,      op_j,        op_jal,
  op_beq,      op_bne,      op_blez,     op_bgtz,
  op_addi,     op_addiu,    op_slti,     op_sltiu,
  op_andi,     op_ori,      op_xori,     op_lui,
  // 0x10
  op_cop0,     op_cop1,     op_cop2,     op_cop3,
  op_illegal,  op_illegal,  op_illegal,  op_illegal,
  op_illegal,  op_illegal,  op_illegal,  op_illegal,
  op_illegal,  op_illegal,  op_illegal,  op_illegal,
  // 0x20
  op_lb,       op_lh,       op_lwl,      op_lw,
  op_lbu,      op_lhu,      op_lwr,      op_illegal,
  op_sb,       op_sh,       op_swl,      op_sw,
  op_illegal,  op_illegal,  op_swr,      op_illegal,
  // 0x30
  op_lwc0,     op_lwc1,     op_lwc2,     op_lwc3,
  op_illegal,  op_illegal,  op_illegal,  op_illegal,
  op_swc0,     op_swc1,     op_swc2,     op_swc3,
  op_illegal,  op_illegal,  op_illegal,  op_illegal,
  // irq active
  // 0x00
  op_irq,      op_irq,      op_irq,      op_irq,
  op_irq,      op_irq,      op_irq,      op_irq,
  op_irq,      op_irq,      op_irq,      op_irq,
  op_irq,      op_irq,      op_irq,      op_irq,
  // 0x10
  op_irq,      op_irq,      op_cop2,     op_irq,
  op_irq,      op_irq,      op_irq,      op_irq,
  op_irq,      op_irq,      op_irq,      op_irq,
  op_irq,      op_irq,      op_irq,      op_irq,
  // 0x20
  op_irq,      op_irq,      op_irq,      op_irq,
  op_irq,      op_irq,      op_irq,      op_irq,
  op_irq,      op_irq,      op_irq,      op_irq,
  op_irq,      op_irq,      op_irq,      op_irq,
  // 0x30
  op_irq,      op_irq,      op_irq,      op_irq,
  op_irq,      op_irq,      op_irq,      op_irq,
  op_irq,      op_irq,      op_irq,      op_irq,
  op_irq,      op_irq,      op_irq,      op_irq,
};

void run_next_ins(Psx *psx) {
  Cpu *cpu = &psx->cpu;

  // Intercept Syscalls
  if (cpu->pc.data == 0xB0) {
    switch (cpu->regs[9] & 0xFF) {
      case 0x3B:
        putchar(cpu->regs[4]);
        break;
      case 0x3D:
        putchar(cpu->regs[4]);
        break;
    }
  }

  cpu->current_pc = cpu->pc;
  cpu->pc = cpu->next_pc;
  cpu->next_pc = MAKE_Addr(cpu->pc.data + 4);

  // Check if in delay_slot
  cpu->delay_slot = cpu->branch;
  cpu->branch = false;

  if (cpu->current_pc.data % 4) {
    exception(psx, LoadAddressError);
    return;
  }

  // Fetch the instruction
  load_sync(cpu);
  tick(psx, MAKE_Cycles(4));
  Ins ins = MAKE_Ins(load(psx, cpu->current_pc, AddrWord));

  instruction_tick(psx);

  // Execute instruction
  OpFunc func = op_handlers[get_func(ins) | psx->cpu.optable_offset]; 
  func(psx, ins);
}
