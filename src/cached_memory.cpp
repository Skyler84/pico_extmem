#include "cached_memory.hpp"
#include "string.h"

#define ASSERT(cond)

CachedMemory::CachedMemory(IMemory *memory)
: m_memory{memory}
, m_next_evict{0}
{

}

uint8_t CachedMemory::read_byte(uintptr_t addr) {
  line_index_t line = cache_line_lookup_fetch(addr&~s_cache_line_addr_mask);
  ASSERT(line != CACHE_MISS);
  return *(uint8_t*)&m_cache_lines[line][addr&s_cache_line_addr_mask];
}

uint16_t CachedMemory::read_word(uintptr_t addr) {
  line_index_t line = cache_line_lookup_fetch(addr&~s_cache_line_addr_mask);
  ASSERT(line != CACHE_MISS);
  return *(uint16_t*)&m_cache_lines[line][addr&s_cache_line_addr_mask];
}

uint32_t CachedMemory::read_dword(uintptr_t addr) {
  line_index_t line = cache_line_lookup_fetch(addr&~s_cache_line_addr_mask);
  ASSERT(line != CACHE_MISS);
  return *(uint32_t*)&m_cache_lines[line][addr&s_cache_line_addr_mask];
}

void CachedMemory::read_data(uintptr_t addr, uint32_t nbytes, uint8_t *data) {
  ASSERT(nbytes < s_cache_line_size);
  ASSERT((nbytes + (addr&s_cache_line_addr_mask)) <= s_cache_line_size);
  line_index_t line = cache_line_lookup_fetch(addr&~s_cache_line_addr_mask);
  ASSERT(line != CACHE_MISS);
  memcpy(data, &m_cache_lines[line][addr&s_cache_line_addr_mask], nbytes);
}

void CachedMemory::write_byte(uintptr_t addr, uint8_t value) {
  line_index_t line = cache_line_lookup_fetch(addr&~s_cache_line_addr_mask);
  ASSERT(line != CACHE_MISS);
  *(uint8_t*)&m_cache_lines[line][addr&s_cache_line_addr_mask] = value;
  m_cache_line_lookups[line].dirty = true;
}

void CachedMemory::write_word(uintptr_t addr, uint16_t value) {
  line_index_t line = cache_line_lookup_fetch(addr&~s_cache_line_addr_mask);
  ASSERT(line != CACHE_MISS);
  *(uint16_t*)&m_cache_lines[line][addr&s_cache_line_addr_mask] = value;
  m_cache_line_lookups[line].dirty = true;
}

void CachedMemory::write_dword(uintptr_t addr, uint32_t value) {
  line_index_t line = cache_line_lookup_fetch(addr&~s_cache_line_addr_mask);
  ASSERT(line != CACHE_MISS);
  *(uint32_t*)&m_cache_lines[line][addr&s_cache_line_addr_mask] = value;
  m_cache_line_lookups[line].dirty = true;
}

void CachedMemory::write_data(uintptr_t addr, uint32_t nbytes, const uint8_t *data) {
  ASSERT(nbytes < s_cache_line_size);
  ASSERT((nbytes + (addr&s_cache_line_addr_mask)) <= s_cache_line_size);
  line_index_t line = cache_line_lookup_fetch(addr&~s_cache_line_addr_mask);
  ASSERT(line != CACHE_MISS);
  memcpy(&m_cache_lines[line][addr&s_cache_line_addr_mask], data, nbytes);
  m_cache_line_lookups[line].dirty = true;
}

CachedMemory::line_index_t CachedMemory::cache_line_lookup(uintptr_t addr) {
  ASSERT(addr&s_cache_line_addr_mask == 0);
  for (line_index_t i = 0; i < s_num_cache_lines; i++) {
    if (m_cache_line_lookups[i].masked_addr == addr)
      return i;
  }
  return CACHE_MISS;
}

CachedMemory::line_index_t CachedMemory::cache_line_lookup_fetch(uintptr_t addr) {
  ASSERT(addr&s_cache_line_addr_mask == 0);
  line_index_t line = cache_line_lookup(addr);
  if (line == CACHE_MISS) {
    //evict cache lines in sequence
    line = m_next_evict;
    m_next_evict++;
    cache_line_evict(line);
    cache_line_fetch(line, addr);
  }
  return line;
}

void CachedMemory::cache_line_fetch(line_index_t line, uintptr_t addr) {
  ASSERT(m_cache_line_lookups[line].dirty == false);
  ASSERT(addr&s_cache_line_addr_mask == 0);
  m_cache_line_lookups[line].masked_addr = addr;
  m_memory->read_data(addr, s_cache_line_size, m_cache_lines[line].data());
}

void CachedMemory::cache_line_evict(line_index_t line) {
  ASSERT(line < s_num_cache_lines);
  if (m_cache_line_lookups[line].dirty) {
    m_memory->write_data(m_cache_line_lookups[line].masked_addr, s_cache_line_size, m_cache_lines[line].data());
  }
  m_cache_line_lookups[line].masked_addr = -1;
  m_cache_line_lookups[line].dirty = false;
}