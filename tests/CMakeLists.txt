add_executable(pico_extmem_test
  test_main.cpp
  profile.cpp
)

target_link_libraries(pico_extmem_test pico_extmem)