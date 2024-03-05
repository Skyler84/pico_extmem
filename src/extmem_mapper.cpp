#include "stdint.h"
#include <array>

#include "extmem_mapper.hpp"
#include <cstdio>
#include "pico/stdio.h"
#include "hardware/exception.h"

#ifndef DEBUG
#define DEBUG 1
#endif

#define PRINT(...) if(DEBUG){printf(__VA_ARGS__); fflush(stdout);}

IMemory *ExtmemMapper::s_memory;
uintptr_t ExtmemMapper::s_base_addr;

enum Registers {
  R0 = 0,
  R1 = 1,
  R2 = 2,
  R3 = 3,
  R4 = 4,
  R5 = 5,
  R6 = 6,
  R7 = 7,
  R8 = 8,
  R9 = 9,
  R10 = 10,
  R11 = 11,
  R12 = 12,
  SP = 13,
  LR = 14,
  PC = 15,
};

struct exception_pushstack{
  uintptr_t R4, R5, R6, R7, _LR, R0, R1, R2, R3, R12, LR, PC, XPSR;
  static constexpr int regs_base = 12;
  static constexpr int regs_mapping[16] = {
    regs_base-7, // r0
    regs_base-6, // r1
    regs_base-5, // r2
    regs_base-4, // r3
    regs_base-12,// r4
    regs_base-11,// r5
    regs_base-10,// r6
    regs_base-9, // r7
    0,
    0,
    0,
    0,
    0,
    0,
    regs_base-2, // lr
    regs_base-1, // pc
  };
} __packed;


struct OpCodeType{
  const char *type;
  void (*handle)(uint16_t, exception_pushstack*);
};

uintptr_t reg_get_value(uint8_t reg, exception_pushstack *ps) {
  if (reg == Registers::SP) return (uintptr_t(ps)) + exception_pushstack::regs_base*sizeof(uintptr_t);
  return ((uintptr_t*)(ps))[exception_pushstack::regs_mapping[reg]];
}

void reg_set_value(uint8_t reg, exception_pushstack *ps, uint32_t value) {
  ((uintptr_t*)(ps))[exception_pushstack::regs_mapping[reg]] = value;
}

void handle_ld_st_single(uint16_t opcode, exception_pushstack *ps) {
  uint8_t reg;
  uintptr_t addr;
  uint8_t opa = (opcode>>12)&0b1111;
  uint8_t opb = (opcode>>9)&0b111;
  bool ld_nst;
  if (opcode <= 0b0101'111'000000000) {
    uint8_t rm = (opcode>>6)&0b111;
    uint8_t rn = (opcode>>3)&0b111;
    reg = (opcode>>0)&0b111;
    PRINT("rm %1x rn %1x reg %1x\n", rm, rn, reg);
    addr = reg_get_value(rn, ps) + reg_get_value(rm, ps);
  }else if (opcode <= 0b0110'111'000000000) {
    uint8_t imm5 = (opcode>>6)&0b11111;
    uint8_t rn = (opcode>>3)&0b111;
    ld_nst = opcode&0b00001000'00000000;
    reg = (opcode>>0)&0b111;
    PRINT("imm5 %02x rn %1x reg %1x\n", imm5, rn, reg);
    addr = reg_get_value(rn, ps) + imm5;
  }else {
    reg = (opcode>>8)&0b111;
    uint8_t imm8 = (opcode>>0)&0b11111111;
    addr = reg_get_value(Registers::SP, ps) + imm8;
    PRINT("imm8 %02x reg %1x\n", imm8, reg);
  }
  PRINT("addr %p reg %1x\n", addr, reg);
  if (ld_nst) {
    PRINT("load from %p into r%d\n", addr, reg);
    addr -= ExtmemMapper::s_base_addr;
    uint32_t regval = 0;
    if (opa == 0b0101)
      switch(opb) {
        case 0b011: regval = (int8_t)ExtmemMapper::s_memory->read_byte(addr); break;
        case 0b100: regval = ExtmemMapper::s_memory->read_dword(addr); break;
        case 0b101: regval = ExtmemMapper::s_memory->read_word(addr); break;
        case 0b110: regval = ExtmemMapper::s_memory->read_byte(addr); break;
        case 0b111: regval = (int16_t)ExtmemMapper::s_memory->read_word(addr); break;
      }
    else if (opa == 0b0110)
      regval = ExtmemMapper::s_memory->read_dword(addr);
    reg_set_value(reg, ps, regval);
  } else {
    uint32_t regval = reg_get_value(reg, ps);
    PRINT("store %08lx from r%d into %p\n", regval, reg, addr);
    addr -= ExtmemMapper::s_base_addr;
    if (opa == 0b0101)
      switch(opb) {
        case 0b000: ExtmemMapper::s_memory->write_dword(addr, regval); break;
        case 0b001: ExtmemMapper::s_memory->write_word(addr, regval); break;
        case 0b010: ExtmemMapper::s_memory->write_byte(addr, regval); break;
      }
    else if (opa == 0b0110)
      ExtmemMapper::s_memory->write_dword(addr, regval);
  }
}

constexpr std::array<OpCodeType, 64> construct_opcode_table(){
  std::array<OpCodeType, 64> out{};
  for (int i = 0; i < 16; i++) {
    out[i] = {"sh/add/sub/mov/cmp"};
  }
  out[0b010000] = {"data proc"};
  out[0b010001] = {"spec/branch"};
  out[0b010010] = {"load lit"};
  out[0b010011] = {"load lit"};
  for (int i = 0b0101'00; i <= 0b100'111; i++) {
    out[i] = OpCodeType{"load/st single", &handle_ld_st_single};
  }
  out[0b101000] = {"gen pc rel"};
  out[0b101001] = {"gen pc rel"};
  out[0b101010] = {"gen sp rel"};
  out[0b101011] = {"gen sp rel"};
  for (int i = 0b1011'00; i <= 0b1011'11; i++) {
    out[i] = {"misc"};
  }
  for (int i = 0b11000'0; i <= 0b11000'1; i++) {
    out[i] = {"st mult"};
  }
  for (int i = 0b11001'0; i <= 0b11001'1; i++) {
    out[i] = {"ld mult"};
  }
  for (int i = 0b1101'00; i <= 0b1101'11; i++) {
    out[i] = {"cond br/svc"};
  }
  for (int i = 0b11100'0; i <= 0b11100'1; i++) {
    out[i] = {"br"};
  }

  return out;
}

auto opcode_types = construct_opcode_table();


void ExtmemMapper::handle_hardfault(void *_ps) {
  exception_pushstack *ps = (exception_pushstack*)_ps;
  PRINT("hardfault\n");
  for (int i = 0; i < 8; i++) {
    PRINT("r%d %08lx\n", i, reg_get_value(i, ps));
  }
  PRINT("lr %p\n", reg_get_value(Registers::LR, ps));
  PRINT("sp %p\n", reg_get_value(Registers::SP, ps));
  PRINT("pc %p\n", reg_get_value(Registers::PC, ps));
  uint16_t cur_thumb_instr = *(uint16_t*)(ps->PC);
  PRINT("%s\n", opcode_types[cur_thumb_instr>>10].type);

  ps->PC += sizeof(uint16_t);

  if (opcode_types[cur_thumb_instr>>10].handle) {
    (*opcode_types[cur_thumb_instr>>10].handle)(cur_thumb_instr, ps);
  }
}

__attribute__((naked))
void ExtmemMapper::hardfault_handler(void) {

  asm volatile(
    "push {r4, r5, r6, r7, lr}\n\t"
    "mov r0, sp\n\t"
    "bl handle_hardfault\n\t"
    "pop {r4, r5, r6, r7, pc}\n\t"
  );
}


void ExtmemMapper::init(IMemory *memory, uintptr_t base)
{
  s_memory = memory;
  s_base_addr = base;
  exception_set_exclusive_handler(HARDFAULT_EXCEPTION, hardfault_handler);
}