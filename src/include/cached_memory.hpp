#pragma once

#include "mem_interface.hpp"
#include <array>
#include <memory>

class CachedMemory final : public IMemory {
public:
  uint8_t read_byte(uintptr_t addr) final override;
  uint16_t read_word(uintptr_t addr) final override;
  uint32_t read_dword(uintptr_t addr) final override;
  void read_data(uintptr_t addr, uint32_t nbytes, uint8_t *data) final override;

  void write_byte(uintptr_t addr, uint8_t value) final override;
  void write_word(uintptr_t addr, uint16_t value) final override;
  void write_dword(uintptr_t addr, uint32_t value) final override;
  void write_data(uintptr_t addr, uint32_t nbytes, uint8_t const *data) final override;

  uint32_t max_read() const final override { return s_cache_line_size; }
  uint32_t max_write() const final override { return s_cache_line_size; }

  uint32_t size_bytes() const final override { return m_memory->size_bytes(); }
  
protected:
private:
  static constexpr unsigned int s_num_cache_lines = 32;
  static constexpr unsigned int s_cache_line_size_pow2 = 5;
  static constexpr unsigned int s_cache_line_size = 1<<s_cache_line_size_pow2;
  static constexpr unsigned int s_cache_line_addr_mask = s_cache_line_size-1;

  std::array<
    std::array<uint8_t, s_cache_line_size>, 
    s_num_cache_lines
  > m_cache_lines;

  struct CacheLineData{
    uintptr_t masked_addr;
    bool dirty;
  };

  std::array<
    int,
    s_num_cache_lines
  > m_cache_line_lookups;

  const std::unique_ptr<IMemory> m_memory;

};