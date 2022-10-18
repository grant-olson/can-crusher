cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(can-crusher C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(can-crusher
  can-crusher.c
  led.c
  button.c
  motor.c
)

pico_enable_stdio_usb(can-crusher 1)
# Must be off or it interferes with talking to TMC2209
pico_enable_stdio_uart(can-crusher 0)

pico_add_extra_outputs(can-crusher)

target_link_libraries(can-crusher pico_stdlib)
