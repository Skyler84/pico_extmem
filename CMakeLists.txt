cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

# include(pico_extras_import_optional.cmake)

project(pico_extmem C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        )
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-maybe-uninitialized)
endif()


add_library(pico_extmem 
    src/spiram.cpp
    src/extmem_mapper.cpp
    src/cached_memory.cpp
)
target_include_directories(pico_extmem PUBLIC src/include)
target_compile_definitions(pico_extmem PRIVATE DEBUG=0)
target_link_libraries(pico_extmem pico_stdlib pico_stdio_usb hardware_exception hardware_spi)

add_subdirectory(examples/)
