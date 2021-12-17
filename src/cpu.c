#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "log.h"
#include "interconnect.h"
#include "instruction.h"
#include "output_logger.h"
#include "flag.h"

typedef struct RomHeader {
  Addr initial_pc;
  uint32_t initial_r28;
  Addr ram_address;
  Addr data_start;
  uint32_t data_size;
  Addr bss_start;
  uint32_t bss_size;
  Addr r29_start;
  uint32_t r29_size;
} RomHeader;

void side_load_rom(Cpu *cpu) {
  FILE * const fp = fopen(get_rom_filename(), "rb");

  if (!fp)
    fatal("IOError: Could not open ROM: %s", rom_filename);

  RomHeader header;

  fseek(fp, 0x10L, SEEK_SET);
  if (fread(&header, sizeof(RomHeader), 1, fp) != 1)
    fatal("IOError: Invalid ROM Header: %s", rom_filename);

  cpu->next_pc = header.initial_pc;
  cpu->regs[28] = header.initial_r28;
  header.ram_address = mask_region(header.ram_address);

  fseek(fp, 0x0L, SEEK_END);
  uint64_t count = ftell(fp) - 1;
  fseek(fp, 0x800L, SEEK_SET);
  count -= ftell(fp);

  if (fread(&cpu->inter.ram.data[header.ram_address.data], sizeof(uint8_t), count, fp) != sizeof(uint8_t) * count)
    fatal("IOError: Couldn't read ROM data: %s", rom_filename);

  memset(&cpu->inter.ram.data[header.data_start.data], 0, header.data_size);
  memset(&cpu->inter.ram.data[header.bss_start.data], 0, header.bss_size);
  if (header.r29_start.data) {
    cpu->regs[29] = header.r29_start.data + header.r29_size;
    cpu->regs[30] = header.r29_start.data + header.r29_size;
  }

  fclose(fp);
}

Cpu init_cpu(char const *bios_filename) {
  Cpu cpu = {0};

  cpu.pc = MAKE_Addr(0xBFC00000);
  cpu.next_pc = MAKE_Addr(cpu.pc.data + 4);
  cpu.current_pc = MAKE_Addr(0x0);
  for(int index = 0; index < 32; index++) {
    cpu.regs[index] = 0xDEADDEAD;
  }
  cpu.regs[0] = 0x0;
  cpu.bad_v_adr = MAKE_Addr(0x0);
  cpu.sr = 0x0;
  cpu.cause = 0x0;
  cpu.epc = MAKE_Addr(0x0);
  cpu.hi = cpu.lo = 0xDEADDEAD;
  cpu.load_delay_slot = MAKE_LoadDelaySlot(MAKE_RegIndex(0x0), 0x0); 
  cpu.inter = init_interconnect(bios_filename);
  cpu.branch = false;
  cpu.delay_slot = false;
  cpu.output_log_index = init_output_log();

  return cpu;
}

void set_reg(Cpu *cpu, RegIndex index, uint32_t value) {
  cpu->regs[index.data] = value;
  cpu->regs[0] = 0x0;
}

void run_next_ins(Cpu *cpu) {
  // Sideload ROM
  if (cpu->pc.data == 0x80030000 && strlen(get_rom_filename()))
    side_load_rom(cpu);

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

  // Fetch the instruction
  Ins ins = MAKE_Ins(load(&cpu->inter, cpu->pc, AddrWord));
  cpu->current_pc = cpu->pc;

  set_pc(cpu->current_pc);
  set_ins(ins);
  if (logging_pc) {
    LOG_PC();
  }
  char const *func_name = funcs[get_func(ins)];
  if (strcmp(func_name, "") == 0)
    func_name = special_funcs[get_sub_func(ins)];

  LOG_OUTPUT(cpu->output_log_index, "%08x %08x: %s", cpu->current_pc.data, ins.data, func_name);

  if (cpu->current_pc.data % 4) {
    exception(cpu, LoadAddressError);
    return;
  }

  // Increment PC
  cpu->pc = cpu->next_pc;
  cpu->next_pc = MAKE_Addr(cpu->next_pc.data + 4);

  // Check if in delay_slot
  cpu->delay_slot = cpu->branch;
  cpu->branch = false;

  if (get_flag(PRINT_INS))
    log_ins(ins);

  // Execute current instruction
  decode_and_execute(cpu, ins);

  print_output_log(cpu->output_log_index);
}

void exception(Cpu *cpu, Exception exp) {
  Addr handler_addr = MAKE_Addr((cpu->sr & (1 << 22)) ? 0xBFC00180 : 0x80000080);

  uint32_t mode = cpu->sr & 0x3F;
  cpu->sr = cpu->sr & ~0x3F;
  cpu->sr = cpu->sr | ((mode << 2) & 0x3F);

  cpu->cause &= ~0x7C;
  cpu->cause |= exp << 2;
  cpu->epc = cpu->current_pc;

  if (cpu->delay_slot) {
    cpu->epc = MAKE_Addr(cpu->epc.data - 4);
    cpu->cause = cpu->cause | (1 << 31);
  } else {
    cpu->epc = cpu->current_pc;
    cpu->cause = cpu->cause & ~(1 << 31);
  }

  cpu->pc = handler_addr;
  cpu->next_pc = MAKE_Addr(cpu->pc.data + 4);
}

void branch(Cpu *cpu, uint32_t offset) {
  offset = offset << 2;

  cpu->next_pc = MAKE_Addr(cpu->pc.data + offset);

  cpu->branch = true;
}

void op_lui(Cpu *cpu, Ins ins) {
  uint32_t imm = get_imm(ins);
  RegIndex rt = get_rt(ins);

  delayed_load(cpu);

  set_reg(cpu, rt, imm << 16);
}

void op_ori(Cpu *cpu, Ins ins) {
  uint32_t imm = get_imm(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  uint32_t reg_s = cpu->regs[rs.data];

  delayed_load(cpu);

  set_reg(cpu, rt, imm | reg_s);
}

void op_xori(Cpu *cpu, Ins ins) {
  uint32_t imm = get_imm(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  uint32_t reg_s = cpu->regs[rs.data];

  delayed_load(cpu);

  set_reg(cpu, rt, imm ^ reg_s);
}

void op_andi(Cpu *cpu, Ins ins) {
  uint32_t imm = get_imm(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  uint32_t reg_s = cpu->regs[rs.data];

  delayed_load(cpu);

  set_reg(cpu, rt, imm & reg_s);
}

void op_sw(Cpu *cpu, Ins ins) {
  if (cpu->sr & 0x10000) {
    log_info("Ignoring store calls while cache is isolated");
    return;
  }

  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  Addr addr = MAKE_Addr(imm_se + cpu->regs[rs.data]);
  uint32_t reg_t = cpu->regs[rt.data];

  delayed_load(cpu);

  if (addr.data % 4) {
    exception(cpu, StoreAddressError);
    return;
  }

  LOG_OUTPUT(cpu->output_log_index, " Addr: 0x%08X, Value: 0x%08X", addr.data, reg_t);

  store(&cpu->inter, addr, reg_t, AddrWord);
}

void op_sll(Cpu *cpu, Ins ins) {
  uint32_t shift = get_shift(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);
  uint32_t reg_t = cpu->regs[rt.data];

  delayed_load(cpu);

  set_reg(cpu, rd, reg_t << shift);
}

void op_sllv(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);
  uint32_t reg_s = cpu->regs[rs.data];
  uint32_t reg_t = cpu->regs[rt.data];

  delayed_load(cpu);

  set_reg(cpu, rd, reg_t << (reg_s & 0x1F));
}

void op_addiu(Cpu *cpu, Ins ins) {
  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  uint32_t reg_s = cpu->regs[rs.data];

  delayed_load(cpu);

  set_reg(cpu, rt, reg_s + imm_se);
}

void op_addi(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  uint32_t imm_se = get_imm_se(ins);
  uint32_t res = imm_se + cpu->regs[rs.data];
  bool overflow = ((cpu->regs[rs.data] ^ res) & (imm_se ^ res)) >> 31;

  delayed_load(cpu);

  if (overflow) {
    exception(cpu, Overflow);
    return;
  }

  set_reg(cpu, rt, res);
}

void op_j(Cpu *cpu, Ins ins) {
  uint32_t imm_jump = get_imm_jump(ins);
  cpu->branch = true;

  cpu->next_pc = MAKE_Addr((cpu->next_pc.data & 0xF0000000) | (imm_jump << 2));

  delayed_load(cpu);
}

void op_jal(Cpu *cpu, Ins ins) {
  uint32_t ra = cpu->next_pc.data;
  op_j(cpu, ins);
  set_reg(cpu, MAKE_RegIndex(0x1F), ra);
}

void op_jalr(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rd = get_rd(ins);

  uint32_t ra = cpu->next_pc.data;
  cpu->next_pc = MAKE_Addr(cpu->regs[rs.data]);

  delayed_load(cpu);

  set_reg(cpu, rd, ra);
  cpu->branch = true;
}

void op_or(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);
  uint32_t reg_s = cpu->regs[rs.data];
  uint32_t reg_t = cpu->regs[rt.data];

  delayed_load(cpu);

  set_reg(cpu, rd, reg_s | reg_t);
}

void op_and(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);
  uint32_t reg_s = cpu->regs[rs.data];
  uint32_t reg_t = cpu->regs[rt.data];

  delayed_load(cpu);

  set_reg(cpu, rd, reg_s & reg_t);
}

void op_nor(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);
  uint32_t reg_s = cpu->regs[rs.data];
  uint32_t reg_t = cpu->regs[rt.data];

  delayed_load(cpu);

  set_reg(cpu, rd, ~(reg_s | reg_t));
}

void op_xor(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);
  uint32_t reg_s = cpu->regs[rs.data];
  uint32_t reg_t = cpu->regs[rt.data];

  delayed_load(cpu);

  set_reg(cpu, rd, reg_s ^ reg_t);
}

void op_mtc0(Cpu *cpu, Ins ins) {
  RegIndex rt = get_rt(ins);
  uint8_t cop_reg = get_cop_reg(ins);
  uint32_t val = cpu->regs[rt.data];

  delayed_load(cpu);

  switch (cop_reg) {
    case 3:
    case 5:
    case 6:
    case 7:
    case 9:
    case 11:
      if (val != 0) {
        fatal("Unhandled write to cop0 register. RegIndex: %d, Val: %d", cop_reg, val);
      }
      break;
    case 12:
      cpu->sr = val;
      break;
    case 13:
      cpu->cause &= ~0x300;
      cpu->cause |= val & 0x300;
      break;
    default:
      fatal("Unhandled write to cop0 register. RegIndex: %d", cop_reg);
  }
}

void op_mfc0(Cpu *cpu, Ins ins) {
  RegIndex rt = get_rt(ins);
  uint8_t cop_reg = get_cop_reg(ins);

  switch (cop_reg) {
    case 6:
    case 7:
      delayed_load_chain(cpu, rt, 0x0);
      break;
    case 8:
      delayed_load_chain(cpu, rt, cpu->bad_v_adr.data);
      break;
    case 12:
      delayed_load_chain(cpu, rt, cpu->sr);
      break;
    case 13:
      delayed_load_chain(cpu, rt, cpu->cause);
      break;
    case 14:
      delayed_load_chain(cpu, rt, cpu->epc.data);
      break;
    case 15:
      // Default PRID Value
      delayed_load_chain(cpu, rt, 0x2);
      break;
    default:
      fatal("Unhandled read from cop0 register. RegIndex: %d", cop_reg);
  }
}

void op_lwc2(Cpu *cpu, Ins ins) {
  fatal("Unhandled GTE LWC. Ins: 0x%08X", ins);
}

void op_swc2(Cpu *cpu, Ins ins) {
  fatal("Unhandled GTE SWC. Ins: 0x%08X", ins);
}

void op_rfe(Cpu *cpu, Ins ins) {
  delayed_load(cpu);

  if ((ins.data & 0x3F) != 0x10)
    fatal("Invalid cop0 instruction: 0x%08X", ins);

  uint32_t mode = cpu->sr & 0x3F;
  cpu->sr = cpu->sr & ~0xF;
  cpu->sr = cpu->sr | (mode >> 2);
}

void op_beq(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  uint32_t imm_se = get_imm_se(ins);

  if (cpu->regs[rs.data] == cpu->regs[rt.data])
    branch(cpu, imm_se);

  delayed_load(cpu);
}

void op_bne(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  uint32_t imm_se = get_imm_se(ins);

  if (cpu->regs[rs.data] != cpu->regs[rt.data])
    branch(cpu, imm_se);

  delayed_load(cpu);
}

void op_blez(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  uint32_t imm_se = get_imm_se(ins);

  int32_t reg_s = cpu->regs[rs.data];

  if (reg_s <= 0)
    branch(cpu, imm_se);

  delayed_load(cpu);
}

void op_bgtz(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  uint32_t imm_se = get_imm_se(ins);

  int32_t reg_s = cpu->regs[rs.data];

  if (reg_s > 0)
    branch(cpu, imm_se);

  delayed_load(cpu);
}

void op_lw(Cpu *cpu, Ins ins) {
  if (cpu->sr & 0x10000) {
    log_info("Ignoring load calls while cache is isolated");
    return;
  }

  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);

  Addr addr = MAKE_Addr(cpu->regs[rs.data] + imm_se);

  if (addr.data % 4) {
    delayed_load(cpu);
    exception(cpu, LoadAddressError);
    return;
  }

  delayed_load_chain(cpu, rt, load(&cpu->inter, addr, AddrWord));
}

void op_lb(Cpu *cpu, Ins ins) {
  if (cpu->sr & 0x10000) {
    log_info("Ignoring load calls while cache is isolated");
    return;
  }

  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);

  Addr addr = MAKE_Addr(cpu->regs[rs.data] + imm_se);
  int8_t data = load(&cpu->inter, addr, AddrByte);
  delayed_load_chain(cpu, rt, data);
}

void op_lbu(Cpu *cpu, Ins ins) {
  if (cpu->sr & 0x10000) {
    log_info("Ignoring load calls while cache is isolated");
    return;
  }

  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);

  Addr addr = MAKE_Addr(cpu->regs[rs.data] + imm_se);
  uint8_t data = load(&cpu->inter, addr, AddrByte);
  delayed_load_chain(cpu, rt, data);
}

void op_sltu(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);
  uint32_t reg_s = cpu->regs[rs.data];
  uint32_t reg_t = cpu->regs[rt.data];

  delayed_load(cpu);

  set_reg(cpu, rd, reg_s < reg_t);
}

void op_slt(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);
  int32_t reg_s = cpu->regs[rs.data];
  int32_t reg_t = cpu->regs[rt.data];

  delayed_load(cpu);

  set_reg(cpu, rd, reg_s < reg_t);
}

void op_slti(Cpu *cpu, Ins ins) {
  int32_t imm_se = get_imm_se(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  int32_t reg_s = cpu->regs[rs.data];

  delayed_load(cpu);

  set_reg(cpu, rt, reg_s < imm_se);
}

void op_sltiu(Cpu *cpu, Ins ins) {
  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  uint32_t reg_s = cpu->regs[rs.data];

  delayed_load(cpu);

  set_reg(cpu, rt, reg_s < imm_se);
}

void op_add(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);
  uint32_t res = cpu->regs[rs.data] + cpu->regs[rt.data];
  bool overflow = ((cpu->regs[rs.data] ^ res) & (cpu->regs[rt.data] ^ res)) >> 31;

  delayed_load(cpu);

  if (overflow) {
    exception(cpu, Overflow);
    return;
  }

  set_reg(cpu, rd, res);
}

void op_addu(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);
  uint32_t reg_s = cpu->regs[rs.data];
  uint32_t reg_t = cpu->regs[rt.data];

  delayed_load(cpu);

  set_reg(cpu, rd, reg_s + reg_t);
}

void op_sh(Cpu *cpu, Ins ins) {
  if (cpu->sr & 0x10000) {
    log_info("Ignoring store calls while cache is isolated");
    return;
  }

  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  uint32_t imm_se = get_imm_se(ins);
  Addr addr = MAKE_Addr(cpu->regs[rs.data] + imm_se);
  uint32_t reg_t = cpu->regs[rt.data];

  delayed_load(cpu);

  if (addr.data % 2) {
    exception(cpu, StoreAddressError);
    return;
  }

  LOG_OUTPUT(cpu->output_log_index, " Addr: %08x, NewValue: %08x",
        addr.data, reg_t
  );
  store(&cpu->inter, addr, reg_t, AddrHalf);
}

void op_sb(Cpu *cpu, Ins ins) {
  if (cpu->sr & 0x10000) {
    log_info("Ignoring store calls while cache is isolated");
    return;
  }

  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  uint32_t imm_se = get_imm_se(ins);

  Addr addr = MAKE_Addr(cpu->regs[rs.data] + imm_se);
  uint32_t reg_t = cpu->regs[rt.data];

  delayed_load(cpu);

  LOG_OUTPUT(cpu->output_log_index, " Addr: %08x, NewValue: %08x",
        addr.data, reg_t
  );
  store(&cpu->inter, addr, reg_t, AddrByte);
}

void op_jr(Cpu *cpu, Ins ins) {
  cpu->next_pc = MAKE_Addr(cpu->regs[get_rs(ins).data]);
  delayed_load(cpu);
  cpu->branch = true;
}

void op_bxx(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  uint32_t imm_se = get_imm_se(ins);

  int is_bgez = (ins.data >> 16) & 0x1 ? 1 : 0;
  int is_link = (((ins.data >> 17) & 0xf) == 0x8) ? 1 : 0;

  int32_t reg_s = cpu->regs[rs.data];
  uint32_t test = (reg_s < 0);
  test = test ^ is_bgez;

  delayed_load(cpu);

  if (is_link)
    set_reg(cpu, MAKE_RegIndex(0x1F), cpu->next_pc.data);

  if (test != 0) {
    branch(cpu, imm_se);
  }
}

void op_sub(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);
  uint32_t res = cpu->regs[rs.data] - cpu->regs[rt.data];
  bool overflow = ((cpu->regs[rs.data] ^ res) & ~(cpu->regs[rt.data] ^ res)) >> 31;

  delayed_load(cpu);

  if (overflow) {
    exception(cpu, Overflow);
    return;
  }

  set_reg(cpu, rd, res);
}

void op_subu(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);
  uint32_t reg_s = cpu->regs[rs.data];
  uint32_t reg_t = cpu->regs[rt.data];

  delayed_load(cpu);

  set_reg(cpu, rd, reg_s - reg_t);
}

void op_sra(Cpu *cpu, Ins ins) {
  uint32_t shift = get_shift(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);

  int32_t reg_t = cpu->regs[rt.data];
  reg_t = reg_t >> shift;

  delayed_load(cpu);

  set_reg(cpu, rd, reg_t);
}

void op_srav(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);

  int32_t reg_t = cpu->regs[rt.data];
  uint8_t shift = cpu->regs[rs.data] & 0x1F;
  reg_t = reg_t >> shift;

  delayed_load(cpu);

  set_reg(cpu, rd, reg_t);
}

void op_srl(Cpu *cpu, Ins ins) {
  uint32_t shift = get_shift(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);
  uint32_t reg_t = cpu->regs[rt.data];

  delayed_load(cpu);

  set_reg(cpu, rd, reg_t >> shift);
}

void op_srlv(Cpu *cpu, Ins ins) {
  RegIndex rs =get_rs(ins);
  RegIndex rt = get_rt(ins);
  RegIndex rd = get_rd(ins);

  uint32_t shift = cpu->regs[rs.data] & 0x1F;
  uint32_t reg_t = cpu->regs[rt.data];

  delayed_load(cpu);

  set_reg(cpu, rd, reg_t >> shift);
}

void op_lh(Cpu *cpu, Ins ins) {
  if (cpu->sr & 0x10000) {
    log_info("Ignoring store calls while cache is isolated");
    return;
  }

  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  uint32_t imm_se = get_imm_se(ins);
  Addr addr = MAKE_Addr(cpu->regs[rs.data] + imm_se);

  if (addr.data % 2) {
    delayed_load(cpu);
    exception(cpu, LoadAddressError);
    return;
  }

  int16_t val = load(&cpu->inter, addr, AddrHalf);

  delayed_load_chain(cpu, rt, val);
}

void op_lhu(Cpu *cpu, Ins ins) {
  if (cpu->sr & 0x10000) {
    log_info("Ignoring load calls while cache is isolated");
    return;
  }

  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);

  Addr addr = MAKE_Addr(cpu->regs[rs.data] + imm_se);

  if (addr.data % 2) {
    delayed_load(cpu);
    exception(cpu, LoadAddressError);
    return;
  }

  delayed_load_chain(cpu, rt, load(&cpu->inter, addr, AddrHalf));
}

void op_lwl(Cpu *cpu, Ins ins) {
  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);

  Addr addr = MAKE_Addr(cpu->regs[rs.data] + imm_se);
  uint32_t cur_val;
  if (cpu->load_delay_slot.index.data == rt.data)
    cur_val = cpu->load_delay_slot.val;
  else
    cur_val = cpu->regs[rt.data];

  Addr aligned_addr = MAKE_Addr(addr.data & ~3);
  Ins aligned_ins = MAKE_Ins(load(&cpu->inter, aligned_addr, AddrWord));

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

  delayed_load_chain(cpu, rt, val);
}

void op_lwr(Cpu *cpu, Ins ins) {
  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);

  Addr addr = MAKE_Addr(cpu->regs[rs.data] + imm_se);
  uint32_t cur_val;
  if (cpu->load_delay_slot.index.data == rt.data)
    cur_val = cpu->load_delay_slot.val;
  else
    cur_val = cpu->regs[rt.data];

  Addr aligned_addr = MAKE_Addr(addr.data & ~3);
  Ins aligned_ins = MAKE_Ins(load(&cpu->inter, aligned_addr, AddrWord));

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

  delayed_load_chain(cpu, rt, val);
}

void op_swl(Cpu *cpu, Ins ins) {
  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);

  Addr addr = MAKE_Addr(cpu->regs[rs.data] + imm_se);
  uint32_t val = cpu->regs[rt.data];

  Addr aligned_addr = MAKE_Addr(addr.data & ~3);
  uint32_t cur_val = load(&cpu->inter, aligned_addr, AddrWord);

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

  delayed_load(cpu);

  store(&cpu->inter, aligned_addr, final_val, AddrWord);
}

void op_swr(Cpu *cpu, Ins ins) {
  uint32_t imm_se = get_imm_se(ins);
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);

  Addr addr = MAKE_Addr(cpu->regs[rs.data] + imm_se);
  uint32_t val = cpu->regs[rt.data];

  Addr aligned_addr = MAKE_Addr(addr.data & ~3);
  uint32_t cur_val = load(&cpu->inter, aligned_addr, AddrWord);

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

  delayed_load(cpu);

  store(&cpu->inter, aligned_addr, final_val, AddrWord);
}
void op_mult(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  int32_t reg_s = cpu->regs[rs.data];
  int32_t reg_t = cpu->regs[rt.data];
  int64_t reg_s_64 = reg_s;
  int64_t reg_t_64 = reg_t;

  delayed_load(cpu);

  uint64_t prod = reg_s_64 * reg_t_64;
  cpu->hi = prod >> 32;
  cpu->lo = prod;
}

void op_multu(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  uint64_t reg_s = cpu->regs[rs.data];
  uint64_t reg_t = cpu->regs[rt.data];

  delayed_load(cpu);

  uint64_t prod = reg_s * reg_t;
  cpu->hi = prod >> 32;
  cpu->lo = prod;
}

void op_div(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  int32_t reg_s = cpu->regs[rs.data];
  int32_t reg_t = cpu->regs[rt.data];

  delayed_load(cpu);

  if (reg_t == 0) {
    cpu->hi = reg_s;
    cpu->lo = (reg_s >= 0) ? 0xFFFFFFFF : 0x1;
  } else if (reg_s == 0x80000000 && reg_t == 0xFFFFFFFF) {
    cpu->hi = 0x0;
    cpu->lo = reg_s;
  } else {
    cpu->hi = reg_s % reg_t;
    cpu->lo = reg_s / reg_t;
  }
}

void op_divu(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);
  RegIndex rt = get_rt(ins);
  uint32_t reg_s = cpu->regs[rs.data];
  uint32_t reg_t = cpu->regs[rt.data];

  delayed_load(cpu);

  if (reg_t == 0) {
    cpu->hi = reg_s;
    cpu->lo = 0xFFFFFFFF;
  } else {
    cpu->hi = reg_s % reg_t;
    cpu->lo = reg_s / reg_t;
  }
}

void op_mflo(Cpu *cpu, Ins ins) {
  RegIndex rd = get_rd(ins);

  delayed_load(cpu);

  set_reg(cpu, rd, cpu->lo);
}

void op_mtlo(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);

  cpu->lo = cpu->regs[rs.data];

  delayed_load(cpu);
}

void op_mfhi(Cpu *cpu, Ins ins) {
  RegIndex rd = get_rd(ins);

  delayed_load(cpu);

  set_reg(cpu, rd, cpu->hi);
}

void op_mthi(Cpu *cpu, Ins ins) {
  RegIndex rs = get_rs(ins);

  cpu->hi = cpu->regs[rs.data];

  delayed_load(cpu);
}

void op_syscall(Cpu *cpu, Ins ins) {
  delayed_load(cpu);

  exception(cpu, SysCall);
}

void op_break(Cpu *cpu, Ins ins) {
  delayed_load(cpu);

  exception(cpu, Break);
}

void op_mfc2(Cpu *cpu, Ins ins) {
  log_error("STUB: op_mfc2 unimplemented!");
}

void op_cfc2(Cpu *cpu, Ins ins) {
  log_error("STUB: op_cfc2 unimplemented!");
}

void op_mtc2(Cpu *cpu, Ins ins) {
  log_error("STUB: op_mtc2 unimplemented!");
}

void op_ctc2(Cpu *cpu, Ins ins) {
  log_error("STUB: op_ctc2 unimplemented!");
}

void op_cop0(Cpu *cpu, Ins ins) {
  switch (get_cop_func(ins)) {
    case 0x0:
      op_mfc0(cpu, ins);
      break;
    case 0x4:
      op_mtc0(cpu, ins);
      break;
    case 0x10:
      op_rfe(cpu, ins);
      break;
    default:
      exception(cpu, IllegalInstruction);
  }
}

void op_cop2(Cpu *cpu, Ins ins) {
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
        op_mfc2(cpu, ins);
        break;
      case 0x2:
        op_cfc2(cpu, ins);
        break;
      case 0x4:
        op_mtc2(cpu, ins);
        break;
      case 0x6:
        op_ctc2(cpu, ins);
        break;
      default:
        exception(cpu, IllegalInstruction);
    }
  }
}

void decode_and_execute(Cpu *cpu, Ins ins) {
  switch (get_func(ins)) {
    case 0x0:
      switch (get_sub_func(ins)) {
        case 0x0:
          op_sll(cpu, ins);
          break;
        case 0x2:
          op_srl(cpu, ins);
          break;
        case 0x3:
          op_sra(cpu, ins);
          break;
        case 0x4:
          op_sllv(cpu, ins);
          break;
        case 0x6:
          op_srlv(cpu, ins);
          break;
        case 0x7:
          op_srav(cpu, ins);
          break;
        case 0x8:
          op_jr(cpu, ins);
          break;
        case 0x9:
          op_jalr(cpu, ins);
          break;
        case 0xC:
          op_syscall(cpu, ins);
          break;
        case 0xD:
          op_break(cpu, ins);
          break;
        case 0x10:
          op_mfhi(cpu, ins);
          break;
        case 0x11:
          op_mthi(cpu, ins);
          break;
        case 0x12:
          op_mflo(cpu, ins);
          break;
        case 0x13:
          op_mtlo(cpu, ins);
          break;
        case 0x18:
          op_mult(cpu, ins);
          break;
        case 0x19:
          op_multu(cpu, ins);
          break;
        case 0x1A:
          op_div(cpu, ins);
          break;
        case 0x1B:
          op_divu(cpu, ins);
          break;
        case 0x20:
          op_add(cpu, ins);
          break;
        case 0x21:
          op_addu(cpu, ins);
          break;
        case 0x22:
          op_sub(cpu, ins);
          break;
        case 0x23:
          op_subu(cpu, ins);
          break;
        case 0x24:
          op_and(cpu, ins);
          break;
        case 0x25:
          op_or(cpu, ins);
          break;
        case 0x26:
          op_xor(cpu, ins);
          break;
        case 0x27:
          op_nor(cpu, ins);
          break;
        case 0x2A:
          op_slt(cpu, ins);
          break;
        case 0x2B:
          op_sltu(cpu, ins);
          break;
        default:
          exception(cpu, IllegalInstruction);
      }
      break;
    case 0x1:
      op_bxx(cpu, ins);
      break;
    case 0x2:
      op_j(cpu, ins);
      break;
    case 0x3:
      op_jal(cpu, ins);
      break;
   case 0x4:
      op_beq(cpu, ins);
      break;
    case 0x5:
      op_bne(cpu, ins);
      break;
    case 0x6:
      op_blez(cpu, ins);
      break;
    case 0x7:
      op_bgtz(cpu, ins);
      break;
    case 0x8:
      op_addi(cpu, ins);
      break;
    case 0x9:
      op_addiu(cpu, ins);
      break;
    case 0xA:
      op_slti(cpu, ins);
      break;
    case 0xB:
      op_sltiu(cpu, ins);
      break;
    case 0xC:
      op_andi(cpu, ins);
      break;
    case 0xD:
      op_ori(cpu, ins);
      break;
    case 0xE:
      op_xori(cpu, ins);
      break;
    case 0xF:
      op_lui(cpu, ins);
      break;
    case 0x10:
      op_cop0(cpu, ins);
      break;
    case 0x11:
    case 0x13:
      exception(cpu, CoprocessorError);
      break;
    case 0x12:
      op_cop2(cpu, ins);
      break;
   case 0x20:
      op_lb(cpu, ins);
      break;
    case 0x21:
      op_lh(cpu, ins);
      break;
    case 0x22:
      op_lwl(cpu, ins);
      break;
    case 0x23:
      op_lw(cpu, ins);
      break;
    case 0x24:
      op_lbu(cpu, ins);
      break;
    case 0x25:
      op_lhu(cpu, ins);
      break;
    case 0x26:
      op_lwr(cpu, ins);
      break;
    case 0x28:
      op_sb(cpu, ins);
      break;
    case 0x29:
      op_sh(cpu, ins);
      break;
    case 0x2A:
      op_swl(cpu, ins);
      break;
    case 0x2B:
      op_sw(cpu, ins);
      break;
    case 0x2E:
      op_swr(cpu, ins);
      break;
    case 0x30:
    case 0x31:
    case 0x33:
      exception(cpu, CoprocessorError);
      break;
    case 0x32:
      op_lwc2(cpu, ins);
      break;
    case 0x38:
    case 0x39:
    case 0x3B:
      exception(cpu, CoprocessorError);
      break;
    case 0x3A:
      op_swc2(cpu, ins);
      break;
    default:
      exception(cpu, IllegalInstruction);
  }
}

void destroy_cpu(Cpu *cpu) {
  destroy_interconnect(&cpu->inter);
}
