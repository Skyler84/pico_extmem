#pragma once

#include "mem_interface.hpp"
#include <memory>

class ExtmemMapper {
public:
  static void init(IMemory *memory, uintptr_t base_addr);
  static IMemory * s_memory;
  static uintptr_t s_base_addr;
protected:
private:

  static void hardfault_handler(void) asm("hardfault_handler");
};