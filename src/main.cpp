#include <stdio.h>
#include "pico/stdio.h"
// #include "pico/stlib.h"
#include "pico/time.h"
#include "hardware/exception.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include <array>

#include "spiram.hpp"
#include "extmem_mapper.hpp"

int main(){
  stdio_init_all();
  
  if(watchdog_enable_caused_reboot()) {
    reset_usb_boot(0,0);
  }

  watchdog_enable(2000, 1);

  SpiRam extmem{19, 16, 18, 3};
  ExtmemMapper::init(&extmem, 0x0300'0000);

  for (int i = 10; i; --i) {
    printf("waiting %ds\n", i);
    sleep_ms(1000);
    watchdog_update();
  }


  printf("hardfaulting!\n");
  asm volatile (
    "mov r0, #0\n\t"
    "mov r1, #1");
  (*(volatile int*)(0x30000000)) = 0x5678;
  sleep_ms(100);
  printf("Hardfault handled\n");
  printf("Value at addr 0: %08lx\n", extmem.read_dword(0));
  extmem.write_dword(0, 0x5678);
  printf("Value at addr 0: %08lx\n", extmem.read_dword(0));
  while(true);
}