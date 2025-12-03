cmake_minimum_required(VERSION 3.13...3.27)

# initialize pico-sdk from submodule
# note: this must happen before project()
include(pico-sdk/pico_sdk_init.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_FLAGS_DEBUG "-Og -g3 -fno-inline -fno-omit-frame-pointer")
set(PICO_BOARD pico)

project(thermal-camera)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(lib/mlx90640)

set(CMAKE_BUILD_REPRODUCIBLE OFF)

add_executable(thermal-camera
  src/fonts.c
  src/main.c
  src/st7789.c
  src/st7789_framebuf.c
)

target_include_directories(thermal-camera PUBLIC
  "${CMAKE_CURRENT_LIST_DIR}/include"
)

target_link_libraries(thermal-camera
  pico_stdlib
  pico_multicore
  hardware_spi
  hardware_clocks
  hardware_pio
  hardware_i2c
  pico_cyw43_arch_lwip_threadsafe_background
  mlx90640
)

pico_enable_stdio_usb(thermal-camera 0)
pico_enable_stdio_uart(thermal-camera 1)

pico_add_extra_outputs(thermal-camera)
