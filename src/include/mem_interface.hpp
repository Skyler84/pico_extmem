#pragma once

#include "stdint.h"

class IMemory {
public:

  virtual ~IMemory(){}

  virtual uint8_t read_byte(uintptr_t addr) = 0;
  virtual uint16_t read_word(uintptr_t addr) = 0;
  virtual uint32_t read_dword(uintptr_t addr) = 0;
  virtual void read_data(uintptr_t addr, uint32_t nbytes, uint8_t *data) = 0;

  virtual void write_byte(uintptr_t addr, uint8_t value) = 0;
  virtual void write_word(uintptr_t addr, uint16_t value) = 0;
  virtual void write_dword(uintptr_t addr, uint32_t value) = 0;
  virtual void write_data(uintptr_t addr, uint32_t nbytes, uint8_t const *data) = 0;

  virtual uint32_t max_read() const = 0;
  virtual uint32_t max_write() const = 0;

  virtual uint32_t size_bytes() const = 0;
};

