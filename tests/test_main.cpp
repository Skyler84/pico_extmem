#include <cstdio>
#include <list>
#include <array>
#include "pico/stdio.h"
#include "pico/time.h"
#include "hardware/exception.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"

#include "spiram.hpp"
#include "cached_memory.hpp"
#include "extmem_mapper.hpp"
#include "profile.hpp"

static std::list<std::tuple<IMemory*, const char*>> s_test_memories;
std::array<uint32_t, 4> g_num_iters = {500, 1'000, 5'000, 10'000};

void test_register_imemory(IMemory *mem) {

}

void run_tests() {

}

void run_profiles() {
  int w = 40, wint=11;
  printf("Num Iterations %*s ", w+9, "");
  for (unsigned i = 0; i < g_num_iters.size(); i++){
    if (i) printf(" :");
    printf("% *d", wint, g_num_iters[i]);
  }
  printf("\n");
  for (auto [mem, desc] : s_test_memories) {
    profile_init(*mem);
    printf("\n");
    for (ProfileFunc *pf = ProfileFunc::all(); pf; pf = pf->next()) {
      printf("CPS (%16s:%*.*s): ", desc, w,w, pf->desc());
      for (unsigned i = 0; i < g_num_iters.size(); i++){
        if (i) printf(" :");
        uint32_t cps = profile_cps(pf->func(), g_num_iters[i]);
        printf("% *d", wint, cps);
      }
      printf("\n");
    }
  }
}


int main(){
  stdio_init_all();
  
  if(watchdog_enable_caused_reboot()) {
    reset_usb_boot(0,0);
  }

  watchdog_enable(4000, 1);

  SpiRam extmem{19, 16, 18, 3};
  Cached_32_32 cache1{&extmem};
  Cached_64_32 cache2{&extmem};
  Cached_64_64 cache3{&extmem};

  s_test_memories.push_back({&extmem, "SpiRam"});
  s_test_memories.push_back({&cache1, "Cached_32_32"});
  s_test_memories.push_back({&cache2, "Cached_64_32"});
  s_test_memories.push_back({&cache3, "Cached_64_64"});

  while(!stdio_usb_connected()){
    sleep_ms(1000);
    printf("waiting\n");
    watchdog_update();
  }
  sleep_ms(1000);
  
  run_tests();

  run_profiles();

  printf("Testing and Profiling complete!\n");
  while(true);
}