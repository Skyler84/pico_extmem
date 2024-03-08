#include "mem_interface.hpp"
#include "extmem_mapper.hpp"
#include "hardware/watchdog.h"

#include "pico/time.h"

#include "profile.hpp"


#define LOOPER(name, n, code) \
void name(uint32_t niters) {\
  niters /= n;\
  for (uint32_t i = 0; i < niters; i++) {\
    code\
    watchdog_update();\
  }\
}\
ProfileFunc pf_##name{&name, #name};

#define DWORD(addr) (*(volatile uint32_t*)(addr))
#define DWORD_OFFSET(addr) (*(volatile uint32_t*)(0x3000'0000+(addr)))
#define READ_DWORD_OFFSET DWORD_OFFSET
#define WRITE_DWORD_OFFSET(addr, value) (*(volatile uint32_t*)(0x3000'0000+(addr)) = (value))

#define READMEMIF(addr) (ExtmemMapper::s_memory->read_dword((addr)))
#define WRITEMEMIF(addr, value) (ExtmemMapper::s_memory->write_dword((addr), (value)))

ProfileFunc *ProfileFunc::s_all;

void profile_init(IMemory &mem) {
  ExtmemMapper::init(&mem, 0x3000'0000);
}

void profile_baseline(uint32_t niters) { 
  for (uint32_t i = 0; i < niters; i++) { 
    (*(volatile uint32_t*)(0x2000'0000));
    watchdog_update(); 
  }
}
uint32_t profile_cps(profiler profile_fn, uint32_t niters) {
  uint64_t start = time_us_64();

  (*profile_fn)(niters);

  uint64_t end = time_us_64();
  uint64_t start1 = time_us_64();

  (profile_baseline)(niters);

  uint64_t end1 = time_us_64();
  uint64_t timedelta = end-start-(end1-start1);
  return (uint64_t(1'000'000)*uint64_t(niters))/timedelta;
}

#define ENUM_TEST(name, read_dword, write_dword) \
LOOPER(read_single_addr_##name, 1, {             \
read_dword(0);                                   \
})                                               \
LOOPER(write_single_addr_##name, 1, {            \
write_dword(0, i);                               \
})                                               \
LOOPER(read_write_single_addr_##name, 2, {       \
read_dword(0);                                   \
write_dword(0, i);                               \
})                                               \
LOOPER(read_write_incremental_4_addr_##name, 2, {\
read_dword(0);                                   \
write_dword(0+i*4, i);                           \
})                                               \
LOOPER(read_write_incremental_8_addr_##name, 2, {\
read_dword(0);                                   \
write_dword(0+i*8, i);                           \
})                                               \
LOOPER(write_incremental_8_addr_##name, 1, {     \
write_dword(0+i*8, i);                           \
})                                               \
LOOPER(write_incremental_4_addr_##name, 1, {     \
write_dword(0+i*4, i);                           \
})

ENUM_TEST(hardfault, READ_DWORD_OFFSET, WRITE_DWORD_OFFSET)
ENUM_TEST(memif, READMEMIF, WRITEMEMIF)