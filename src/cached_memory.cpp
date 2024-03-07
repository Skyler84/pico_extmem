#include "cached_memory.hpp"
#include "string.h"
#include "stdio.h"
#include "pico/time.h"


#define SLEEP_MS(ms) {unsigned int _ms=(ms); while(_ms--) for (int i = 0; i < 1000'000; i++) tight_loop_contents();}
#if DEBUG
#define PRINT(...) ({printf(__VA_ARGS__); fflush(stdout); SLEEP_MS(1);})
#define ASSERT(cond) if(!(cond)){printf("Assert failed: %s\n%s:%d", #cond, __FILE__, __LINE__); while(1);}
#else
#define ASSERT(cond)
#define PRINT(...)
#endif


template<unsigned int u1, unsigned int u2>
CachedMemory<u1, u2>::CachedMemory(IMemory *memory)
: m_memory{memory}
, m_next_evict{0}
{
  for (auto &e : m_cache_line_lookups) {
    e.masked_addr = -1;
  }
}

template<unsigned int u1, unsigned int u2>
uint8_t CachedMemory<u1, u2>::read_byte(uintptr_t addr) {
  PRINT("reading byte from %p\n", addr);
  line_index_t line = cache_line_lookup_fetch(addr&~s_cache_line_addr_mask);
  ASSERT(line != CACHE_MISS);
  return *(uint8_t*)&m_cache_lines[line][addr&s_cache_line_addr_mask];
}

template<unsigned int u1, unsigned int u2>
uint16_t CachedMemory<u1, u2>::read_word(uintptr_t addr) {
  PRINT("reading word from %p\n", addr);
  line_index_t line = cache_line_lookup_fetch(addr&~s_cache_line_addr_mask);
  ASSERT(line != CACHE_MISS);
  return *(uint16_t*)&m_cache_lines[line][addr&s_cache_line_addr_mask];
}

template<unsigned int u1, unsigned int u2>
uint32_t CachedMemory<u1, u2>::read_dword(uintptr_t addr) {
  PRINT("reading dword from %p\n", addr);
  line_index_t line = cache_line_lookup_fetch(addr&~s_cache_line_addr_mask);
  PRINT("line %d\n", line);
  ASSERT(line != CACHE_MISS);
  return *(uint32_t*)&m_cache_lines[line][addr&s_cache_line_addr_mask];
}

template<unsigned int u1, unsigned int u2>
void CachedMemory<u1, u2>::read_data(uintptr_t addr, uint32_t nbytes, uint8_t *data) {
  ASSERT(nbytes < s_cache_line_size);
  ASSERT((nbytes + (addr&s_cache_line_addr_mask)) <= s_cache_line_size);
  line_index_t line = cache_line_lookup_fetch(addr&~s_cache_line_addr_mask);
  ASSERT(line != CACHE_MISS);
  memcpy(data, &m_cache_lines[line][addr&s_cache_line_addr_mask], nbytes);
}

template<unsigned int u1, unsigned int u2>
void CachedMemory<u1, u2>::write_byte(uintptr_t addr, uint8_t value) {
  PRINT("writing %02x to %p\n", value, addr);
  line_index_t line = cache_line_lookup_fetch(addr&~s_cache_line_addr_mask);
  ASSERT(line != CACHE_MISS);
  *(uint8_t*)&m_cache_lines[line][addr&s_cache_line_addr_mask] = value;
  m_cache_line_lookups[line].dirty = true;
}

template<unsigned int u1, unsigned int u2>
void CachedMemory<u1, u2>::write_word(uintptr_t addr, uint16_t value) {
  PRINT("writing %04x to %p\n", value, addr);
  line_index_t line = cache_line_lookup_fetch(addr&~s_cache_line_addr_mask);
  ASSERT(line != CACHE_MISS);
  *(uint16_t*)&m_cache_lines[line][addr&s_cache_line_addr_mask] = value;
  m_cache_line_lookups[line].dirty = true;
}

template<unsigned int u1, unsigned int u2>
void CachedMemory<u1, u2>::write_dword(uintptr_t addr, uint32_t value) {
  PRINT("writing %08x to %p\n", value, addr);
  line_index_t line = cache_line_lookup_fetch(addr&~s_cache_line_addr_mask);
  ASSERT(line != CACHE_MISS);
  *(uint32_t*)&m_cache_lines[line][addr&s_cache_line_addr_mask] = value;
  m_cache_line_lookups[line].dirty = true;

  PRINT("wrote %08x to %p\n", value, addr);
}

template<unsigned int u1, unsigned int u2>
void CachedMemory<u1, u2>::write_data(uintptr_t addr, uint32_t nbytes, const uint8_t *data) {
  ASSERT(nbytes < s_cache_line_size);
  ASSERT((nbytes + (addr&s_cache_line_addr_mask)) <= s_cache_line_size);
  line_index_t line = cache_line_lookup_fetch(addr&~s_cache_line_addr_mask);
  ASSERT(line != CACHE_MISS);
  memcpy(&m_cache_lines[line][addr&s_cache_line_addr_mask], data, nbytes);
  m_cache_line_lookups[line].dirty = true;
}

template<unsigned int u1, unsigned int u2>
typename CachedMemory<u1, u2>::line_index_t CachedMemory<u1, u2>::cache_line_lookup(uintptr_t addr) {
  ASSERT((addr&s_cache_line_addr_mask) == 0);
  for (line_index_t i = 0; i < s_num_cache_lines; i++) {
    if (m_cache_line_lookups[i].masked_addr == addr)
      return i;
  }
  return CACHE_MISS;
}

template<unsigned int u1, unsigned int u2>
typename CachedMemory<u1, u2>::line_index_t CachedMemory<u1, u2>::cache_line_lookup_fetch(uintptr_t addr) {
  ASSERT((addr&s_cache_line_addr_mask) == 0);
  line_index_t line = cache_line_lookup(addr);
  if (line == CACHE_MISS) {
    PRINT("CACHE MISS (%p)\n", addr);
    //evict cache lines in sequence
    line = m_next_evict;
    m_next_evict++;
    m_next_evict &= (s_num_cache_lines-1);
    cache_line_evict(line);
    cache_line_fetch(line, addr);
  }
  PRINT("CACHE %p on %d\n", addr, line);
  return line;
}

template<unsigned int u1, unsigned int u2>
void CachedMemory<u1, u2>::cache_line_fetch(line_index_t line, uintptr_t addr) {
  ASSERT(m_cache_line_lookups[line].dirty == false);
  ASSERT((addr&s_cache_line_addr_mask) == 0);
  m_cache_line_lookups[line].masked_addr = addr;
  m_memory->read_data(addr, s_cache_line_size, m_cache_lines[line].data());
}

template<unsigned int u1, unsigned int u2>
void CachedMemory<u1, u2>::cache_line_evict(line_index_t line) {
  ASSERT(line < s_num_cache_lines);
  if (m_cache_line_lookups[line].dirty) {
    m_memory->write_data(m_cache_line_lookups[line].masked_addr, s_cache_line_size, m_cache_lines[line].data());
  }
  m_cache_line_lookups[line].masked_addr = -1;
  m_cache_line_lookups[line].dirty = false;
}