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

void handle_none(uint16_t, exception_pushstack*){}

#define DECODE_REG3(opcode) std::tuple<uint8_t, uint8_t, uint8_t>{(opcode>>6)&0b111,(opcode>>3)&0b111,(opcode>>0)&0b111}

void handle_0101_000_store(uint16_t opcode, exception_pushstack *ps) {
  auto [rm, rn, rt] = DECODE_REG3(opcode);
  auto regval = reg_get_value(rt, ps);
  uintptr_t addr = reg_get_value(rn, ps) + reg_get_value(rm, ps) - ExtmemMapper::s_base_addr;
  ExtmemMapper::s_memory->write_dword(addr, regval);
}
void handle_0101_001_store(uint16_t opcode, exception_pushstack *ps) {
  auto [rm, rn, rt] = DECODE_REG3(opcode);
  auto regval = reg_get_value(rt, ps);
  uintptr_t addr = reg_get_value(rn, ps) + reg_get_value(rm, ps) - ExtmemMapper::s_base_addr;
  ExtmemMapper::s_memory->write_word(addr, regval);
}
void handle_0101_010_store(uint16_t opcode, exception_pushstack *ps) {
  auto [rm, rn, rt] = DECODE_REG3(opcode);
  auto regval = reg_get_value(rt, ps);
  uintptr_t addr = reg_get_value(rn, ps) + reg_get_value(rm, ps) - ExtmemMapper::s_base_addr;
  ExtmemMapper::s_memory->write_byte(addr, regval);
}
void handle_0101_011_load(uint16_t opcode, exception_pushstack *ps) {
  auto [rm, rn, rt] = DECODE_REG3(opcode);
  uintptr_t addr = reg_get_value(rn, ps) + reg_get_value(rm, ps) - ExtmemMapper::s_base_addr;
  int8_t val = ExtmemMapper::s_memory->read_byte(addr);
  reg_set_value(rt, ps, val);
}
void handle_0101_100_load(uint16_t opcode, exception_pushstack *ps) {
  auto [rm, rn, rt] = DECODE_REG3(opcode);
  uintptr_t addr = reg_get_value(rn, ps) + reg_get_value(rm, ps) - ExtmemMapper::s_base_addr;
  uint32_t val = ExtmemMapper::s_memory->read_dword(addr);
  reg_set_value(rt, ps, val);
}
void handle_0101_101_load(uint16_t opcode, exception_pushstack *ps) {
  auto [rm, rn, rt] = DECODE_REG3(opcode);
  uintptr_t addr = reg_get_value(rn, ps) + reg_get_value(rm, ps) - ExtmemMapper::s_base_addr;
  uint16_t val = ExtmemMapper::s_memory->read_word(addr);
  reg_set_value(rt, ps, val);
  }
void handle_0101_110_load(uint16_t opcode, exception_pushstack *ps) {
  auto [rm, rn, rt] = DECODE_REG3(opcode);
  uintptr_t addr = reg_get_value(rn, ps) + reg_get_value(rm, ps) - ExtmemMapper::s_base_addr;
  uint8_t val = ExtmemMapper::s_memory->read_byte(addr);
  reg_set_value(rt, ps, val);
      }
void handle_0101_111_load(uint16_t opcode, exception_pushstack *ps) {
  auto [rm, rn, rt] = DECODE_REG3(opcode);
  uintptr_t addr = reg_get_value(rn, ps) + reg_get_value(rm, ps) - ExtmemMapper::s_base_addr;
  int16_t val = ExtmemMapper::s_memory->read_word(addr);
  reg_set_value(rt, ps, val);
}
#define DECODE_IMM5_REG2(opcode) std::tuple<uint8_t, uint8_t, uint8_t>{(opcode>>6)&0b11111,(opcode>>3)&0b111,(opcode>>0)&0b111}
void handle_0110_0xx_store(uint16_t opcode, exception_pushstack *ps) {
  auto [imm5, rn, rt] = DECODE_IMM5_REG2(opcode);
  uintptr_t addr = reg_get_value(rn, ps) + imm5 - ExtmemMapper::s_base_addr;
  auto regval = reg_get_value(rt, ps);
  ExtmemMapper::s_memory->write_dword(addr, regval);
}
void handle_0110_1xx_load(uint16_t opcode, exception_pushstack *ps) {
  auto [imm5, rn, rt] = DECODE_IMM5_REG2(opcode);
  uintptr_t addr = reg_get_value(rn, ps) + imm5 - ExtmemMapper::s_base_addr;
  auto val = ExtmemMapper::s_memory->read_dword(addr);
  reg_set_value(rt, ps, val);
}
void handle_0111_0xx_store(uint16_t opcode, exception_pushstack *ps) {
  auto [imm5, rn, rt] = DECODE_IMM5_REG2(opcode);
  uintptr_t addr = reg_get_value(rn, ps) + imm5 - ExtmemMapper::s_base_addr;
  auto regval = reg_get_value(rt, ps);
  ExtmemMapper::s_memory->write_byte(addr, regval);
}
void handle_0111_1xx_load(uint16_t opcode, exception_pushstack *ps) {
  auto [imm5, rn, rt] = DECODE_IMM5_REG2(opcode);
  uintptr_t addr = reg_get_value(rn, ps) + imm5 - ExtmemMapper::s_base_addr;
  auto val = ExtmemMapper::s_memory->read_byte(addr);
  reg_set_value(rt, ps, val);
}
void handle_1000_0xx_store(uint16_t opcode, exception_pushstack *ps) {
  auto [imm5, rn, rt] = DECODE_IMM5_REG2(opcode);
  uintptr_t addr = reg_get_value(rn, ps) + imm5 - ExtmemMapper::s_base_addr;
  auto regval = reg_get_value(rt, ps);
  ExtmemMapper::s_memory->write_word(addr, regval);
}
void handle_1000_1xx_load(uint16_t opcode, exception_pushstack *ps) {
  auto [imm5, rn, rt] = DECODE_IMM5_REG2(opcode);
  uintptr_t addr = reg_get_value(rn, ps) + imm5 - ExtmemMapper::s_base_addr;
  auto val = ExtmemMapper::s_memory->read_word(addr);
  reg_set_value(rt, ps, val);
}
#define DECODE_REG_IMM8(opcode) std::tuple<uint8_t, uint8_t>{(opcode>>8)&0b111, opcode&0xff}
void handle_1001_0xx_store(uint16_t opcode, exception_pushstack *ps) {
  auto [rt, imm8] = DECODE_REG_IMM8(opcode);
  uintptr_t addr = reg_get_value(Registers::SP, ps) + imm8 - ExtmemMapper::s_base_addr;
  auto regval = reg_get_value(rt, ps);
  ExtmemMapper::s_memory->write_word(addr, regval);
}
void handle_1001_1xx_load(uint16_t opcode, exception_pushstack *ps) {
  auto [rt, imm8] = DECODE_REG_IMM8(opcode);
  uintptr_t addr = reg_get_value(Registers::SP, ps) + imm8 - ExtmemMapper::s_base_addr;
  auto val = ExtmemMapper::s_memory->read_word(addr);
  reg_set_value(rt, ps, val);
}

constexpr auto construct_opcode_table(){
  std::array<OpCodeType, 128> out{};
  for (auto &e : out) {
    e.handle = handle_none;
    e.type = "NONE";
  }
  for (int i = 0; i < 0b0100000; i++) {
    out[i] = {"sh/add/sub/mov/cmp"};
  }
  out[0b0100000] = {"data proc"};
  out[0b0100001] = {"data proc"};
  out[0b0100010] = {"spec/branch"};
  out[0b0100011] = {"spec/branch"};
  out[0b0100100] = {"load lit"};
  out[0b0100101] = {"load lit"};
  out[0b0100110] = {"load lit"};
  out[0b0100111] = {"load lit"};

  out[0b0101'000] = OpCodeType{"Store Register", &handle_0101_000_store};
  out[0b0101'001] = OpCodeType{"Store Register Halfword", &handle_0101_001_store};
  out[0b0101'010] = OpCodeType{"Store Register Byte", &handle_0101_010_store};
  out[0b0101'011] = OpCodeType{"Load Register Signed Byte", &handle_0101_011_load};
  out[0b0101'100] = OpCodeType{"Load Register", &handle_0101_100_load};
  out[0b0101'101] = OpCodeType{"Load Register Halword", &handle_0101_101_load};
  out[0b0101'110] = OpCodeType{"Load Register Byte", &handle_0101_110_load};
  out[0b0101'111] = OpCodeType{"Load Register Signed Halfword", &handle_0101_111_load};
  for (int i = 0b0110'000; i <= 0b0110'011; i++) out[i] = OpCodeType{"Store Register", &handle_0110_0xx_store};
  for (int i = 0b0110'100; i <= 0b0110'111; i++) out[i] = OpCodeType{"Load Register", &handle_0110_1xx_load};
  for (int i = 0b0111'000; i <= 0b0111'011; i++) out[i] = OpCodeType{"Store Register Byte", &handle_0111_0xx_store};
  for (int i = 0b0111'100; i <= 0b0111'111; i++) out[i] = OpCodeType{"Load Register Byte", &handle_0111_1xx_load};
  for (int i = 0b1000'000; i <= 0b1000'011; i++) out[i] = OpCodeType{"Store Register Halfword", &handle_1000_0xx_store};
  for (int i = 0b1000'100; i <= 0b1000'111; i++) out[i] = OpCodeType{"Load Register Halfword", &handle_1000_1xx_load};
  for (int i = 0b1001'000; i <= 0b1001'011; i++) out[i] = OpCodeType{"Store Register", &handle_1001_0xx_store};
  for (int i = 0b1001'100; i <= 0b1001'111; i++) out[i] = OpCodeType{"Load Register", &handle_1001_1xx_load};
  for (int i = 0b1010000; i <= 0b1010111; i++) {
    out[i] = {"gen pc rel"};
  }
  for (int i = 0b1011'000; i <= 0b1011'111; i++) {
    out[i] = {"misc"};
  }
  for (int i = 0b11000'00; i <= 0b11000'11; i++) {
    out[i] = {"st mult"};
  }
  for (int i = 0b11001'00; i <= 0b11001'11; i++) {
    out[i] = {"ld mult"};
  }
  for (int i = 0b1101'000; i <= 0b1101'111; i++) {
    out[i] = {"cond br/svc"};
  }
  for (int i = 0b11100'00; i <= 0b11100'11; i++) {
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
  int opcode_nbits=7;
  PRINT("%s\n", opcode_types[cur_thumb_instr>>(16-opcode_nbits)].type);

  // ps->PC += sizeof(uint16_t);

  if (opcode_types[cur_thumb_instr>>(16-opcode_nbits)].handle) {
    (*opcode_types[cur_thumb_instr>>(16-opcode_nbits)].handle)(cur_thumb_instr, ps);
  }
}

__attribute__((naked))
void ExtmemMapper::hardfault_handler(void) {

  asm volatile(
    "mov r0, sp\n\t"
    "ldr r0, [r0, #24]\n\t"             // get PC from stack
    "eor r1, r1, r1\n\t"                // clear r1
    "ldrh r0, [r0, r1]\n\t"             // get instruction from pc
    "push {r4, r5, r6, r7, lr}\n\t"     // save regs
    "mov r1, sp\n\t"                    // second function parameter
    "lsr r2, r0, #9 \n\t"               // opcode >> 9
    "lsl r2, r2, #3 \n\t"               // index (*sizeof OpCodeType)
    "add r3, r3, r2\n\t"                // optype+index
    "ldr r3, [r3, #4]\n\t"              // handler = optype[index].handle
    "blx r3\n\t"                        // (*handler)()
    "pop {r4, r5, r6, r7} \n\t"         // restore regs
    "pop {r0}\n\t"                      // pop pc without return
    "ldr r2, [sp, #24]\n\t"             // get PC address from stack
    "add r2, r2, #2\n\t"                // increment PC
    "str r2, [sp, #24]\n\t"             // writeback PC
    "bx r0"                             // return
    :: "r"(&opcode_types)
  );
}


void ExtmemMapper::init(IMemory *memory, uintptr_t base)
{
  s_memory = memory;
  s_base_addr = base;
  exception_set_exclusive_handler(HARDFAULT_EXCEPTION, hardfault_handler);
}