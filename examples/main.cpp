#include <cstdio>
#include "pico/stdio.h"
#include "pico/time.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"

#include "spiram.hpp"
#include "cached_memory.hpp"
#include "extmem_mapper.hpp"

int main(){
  stdio_init_all();
  
  if(watchdog_enable_caused_reboot()) {
    reset_usb_boot(0,0);
  }

  watchdog_enable(4000, 1);

  SpiRam extmem{19, 16, 18, 3};
  Cached_32_32 cache(&extmem);
  ExtmemMapper::init(&cache, 0x3000'0000);

  while(!stdio_usb_connected()){
    sleep_ms(1000);
    printf("waiting\n");
    watchdog_update();
  }
  sleep_ms(1000);
  
  uintptr_t base_addr = 0x3000'0000;
  uintptr_t addr = 0x3000'0000;
  uint32_t value = 0x1234'5678;
  printf("Writing %08x to %p!\n", value, addr);
  *((volatile uint32_t*)(addr)) = value;
  printf("Hardfault handled\n");
  printf("Value at addr %p: %08x\n", addr-base_addr, cache.read_dword(addr-base_addr));

  uint32_t read_addr = 0x3000'8020;
  uint32_t read_test_value = 0xc0de'cafe;
  uint32_t read_value;
  printf("Setting addr %p: %08x\n", read_addr-base_addr, read_test_value);
  cache.write_dword(read_addr-base_addr, read_test_value);
  printf("Reading from %p!\n", read_addr);
  read_value = *((volatile uint32_t*)(read_addr));
  printf("Hardfault handled\n");
  printf("Read %08x from %p\n", read_value, read_addr);

  printf("Done, resetting\n");
  while(true);
}