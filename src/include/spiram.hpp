#pragma once

#include <stdint.h>
#include <pico/stdlib.h>
#include "mem_interface.hpp"

class SpiRam final : public IMemory{
  public:
    SpiRam(uint mosi, uint miso, uint sclk, uint cs);
    ~SpiRam(){}

    uint8_t read_byte(uintptr_t addr);
    uint16_t read_word(uintptr_t addr);
    uint32_t read_dword(uintptr_t addr);
    void read_data(uintptr_t addr, uint32_t nbytes, uint8_t *out);

    void write_byte(uintptr_t addr, uint8_t value);
    void write_word(uintptr_t addr, uint16_t value);
    void write_dword(uintptr_t addr, uint32_t value);
    void write_data(uintptr_t addr, uint32_t nbytes, uint8_t const *data);

    uint32_t max_read() const { return 1024; }
    uint32_t max_write() const { return 1024; }

    uint32_t size_bytes() const { return 0x0080'0000; } // 8MB

  protected:
  private:
    uint cs;
};