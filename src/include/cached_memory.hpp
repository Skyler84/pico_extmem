#pragma once

#include "mem_interface.hpp"
#include <array>
#include <memory>
#include <map>

template<unsigned int ncl, unsigned int clsp2>
class CachedMemory final : public IMemory {
public:

  CachedMemory(IMemory *memory);

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
  
  using line_index_t = unsigned int;
  static constexpr line_index_t CACHE_MISS = -1;
  static constexpr unsigned int s_num_cache_lines = 32;
  static constexpr unsigned int s_cache_line_size_pow2 = 5;
  static constexpr unsigned int s_cache_line_size = 1<<s_cache_line_size_pow2;
  static constexpr unsigned int s_cache_line_addr_mask = s_cache_line_size-1;

  void cache_line_evict(line_index_t line);

protected:
private:
  IMemory *const m_memory;
  line_index_t m_next_evict;

  std::array<
    std::array<uint8_t, s_cache_line_size>, 
    s_num_cache_lines
  > m_cache_lines;

  struct CacheLineData{
    uintptr_t masked_addr;
    bool dirty;
  };

  std::array<
    CacheLineData,
    s_num_cache_lines
  > m_cache_line_lookups;
  
  std::map<uintptr_t, line_index_t> m_cache_line_indexes;

  line_index_t cache_line_lookup(uintptr_t addr);
  line_index_t cache_line_lookup_fetch(uintptr_t addr);
  void cache_line_fetch(line_index_t line, uintptr_t addr);

};

#define TPL_USING(name, tpl, ...) template class tpl<__VA_ARGS__>; using name = tpl<__VA_ARGS__>

TPL_USING(Cached_8_8,    CachedMemory, 8, 3);
TPL_USING(Cached_8_16,   CachedMemory, 8, 4);
TPL_USING(Cached_8_32,   CachedMemory, 8, 5);
TPL_USING(Cached_8_64,   CachedMemory, 8, 6);
TPL_USING(Cached_8_128,  CachedMemory, 8, 7);
TPL_USING(Cached_8_256,  CachedMemory, 8, 8);
TPL_USING(Cached_8_512,  CachedMemory, 8, 9);
TPL_USING(Cached_8_1024, CachedMemory, 8, 10);

TPL_USING(Cached_16_8,    CachedMemory, 16, 3);
TPL_USING(Cached_16_16,   CachedMemory, 16, 4);
TPL_USING(Cached_16_32,   CachedMemory, 16, 5);
TPL_USING(Cached_16_64,   CachedMemory, 16, 6);
TPL_USING(Cached_16_128,  CachedMemory, 16, 7);
TPL_USING(Cached_16_256,  CachedMemory, 16, 8);
TPL_USING(Cached_16_512,  CachedMemory, 16, 9);
TPL_USING(Cached_16_1024, CachedMemory, 16, 10);

TPL_USING(Cached_32_8,    CachedMemory, 32, 3);
TPL_USING(Cached_32_16,   CachedMemory, 32, 4);
TPL_USING(Cached_32_32,   CachedMemory, 32, 5);
TPL_USING(Cached_32_64,   CachedMemory, 32, 6);
TPL_USING(Cached_32_128,  CachedMemory, 32, 7);
TPL_USING(Cached_32_256,  CachedMemory, 32, 8);
TPL_USING(Cached_32_512,  CachedMemory, 32, 9);
TPL_USING(Cached_32_1024, CachedMemory, 32, 10);

TPL_USING(Cached_64_8,    CachedMemory, 64, 3);
TPL_USING(Cached_64_16,   CachedMemory, 64, 4);
TPL_USING(Cached_64_32,   CachedMemory, 64, 5);
TPL_USING(Cached_64_64,   CachedMemory, 64, 6);
TPL_USING(Cached_64_128,  CachedMemory, 64, 7);
TPL_USING(Cached_64_256,  CachedMemory, 64, 8);
TPL_USING(Cached_64_512,  CachedMemory, 64, 9);
TPL_USING(Cached_64_1024, CachedMemory, 64, 10);