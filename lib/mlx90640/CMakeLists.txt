add_library(mlx90640
  src/MLX90640_API.c
  src/MLX90640_I2C_Driver.c
)

target_include_directories(mlx90640 PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(mlx90640
  pico_stdlib
  hardware_i2c
)
