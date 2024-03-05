#include <cstdio>
#include "pico/stdio.h"
#include "pico/time.h"
#include "hardware/exception.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"

#include "spiram.hpp"
#include "cached_memory.hpp"
#include "extmem_mapper.hpp"

void time_hardfaults(){
  uint64_t start = time_us_64();
  uint32_t niters = 5'000;

  for (uint32_t i = 0; i < niters; i++) {
    (*(volatile int*)(0x3000'0000));
    (*(volatile int*)(0x3000'0000)) = i;
    watchdog_update();
  }

  uint64_t end = time_us_64();
  uint64_t timedelta = end-start;
  printf("Time taken for %d hardfaults: %lldus\n%lld hardfaults/second", niters, timedelta, uint64_t((uint64_t(1'000'000)*uint64_t(niters))/timedelta)*2);
}

int main(){
  stdio_init_all();
  
  if(watchdog_enable_caused_reboot()) {
    reset_usb_boot(0,0);
  }

  watchdog_enable(2000, 1);

  SpiRam extmem{19, 16, 18, 3};
  CachedMemory cache{&extmem};
  ExtmemMapper::init(&cache, 0x3000'0000);

  // Allow the usb serial time to connect
  for (int i = 10; i; --i) {
    printf("waiting %ds\n", i);
    sleep_ms(1000);
    watchdog_update();
  }


  printf("hardfaulting!\n");
  //set some registers for us to look at
  asm volatile (
    "mov r0, #0\n\t"
    "mov r1, #1\n\t"
    "str %0, [%1, #0]"
    :: "r" (0x5678), "r" (0x3000'0000));
  // (*(volatile int*)(0x3000'0000)) = 0x5678;

  printf("Hardfault handled\n");
  printf("Value at addr 0: %08lx\n", cache.read_dword(0));
  cache.write_dword(4, 0x1234);
  printf("Reading 0x30000000: %08lx\n", (*(volatile int*)(0x3000'0000)));
  printf("Reading 0x30000004: %08lx\n", (*(volatile int*)(0x3000'0004)));

  time_hardfaults();
  for (int i = 5; i; --i) {
    printf("Cooling down, %ds\n", i);
  }
  while(true);
}