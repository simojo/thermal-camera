/**
 * @copyright (C) 2025 Simon J. Jones <github@simonjjones.com>
 */

#include <stddef.h>
#include <stdio.h>

#include "mlx90640/MLX90640_I2C_Driver.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"

#define I2C_BAUD 100 * 1000
#define MAX_RETRIES 5

bool init = 0;

void MLX90640_I2CInit() {
}

int MLX90640_I2CGeneralReset() {
  i2c_deinit(i2c_default);
  i2c_init(i2c_default, I2C_BAUD);
  return 0;
}

static inline void _i2c_init(void) {
  i2c_init(i2c_default, I2C_BAUD);
  gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
  gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
  // Make the I2C pins available to picotool
  bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
  init = true;
}

int MLX90640_I2CRead(uint8_t slaveAddr, uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *data) {
    if (!init) {
      _i2c_init();
    }

    uint8_t buf[1664];
    uint8_t cmd[2] = {0, 0};

    cmd[0] = startAddress >> 8;
    cmd[1] = startAddress & 0x00FF;

    uint16_t *p = data;

    int error;
    // NOTE: timeout is divided by number of bytes to write/read
    uint write_timeout_us = 1000 * 2;
    uint read_timeout_us = 1000 * 2 * nMemAddressRead;

    for (int i = 0; i <= MAX_RETRIES; i++) {
      if (i == MAX_RETRIES) {
        return -1;
      }
      // error = i2c_write_timeout_us(i2c_default, slaveAddr, cmd, 2, 1, write_timeout_us);
      error = i2c_write_blocking(i2c_default, slaveAddr, cmd, 2, 1);
      if (error == PICO_ERROR_TIMEOUT) {
#ifdef DEBUG
        printf("Error: I2C bus stuck - resetting.\n");
#endif
        i2c_deinit(i2c_default);
        i2c_init(i2c_default, I2C_BAUD);
        continue;
      } else if (error < 0) {
        printf("[ERROR] I2CRead0: %d\n", error);
        i2c_deinit(i2c_default);
        i2c_init(i2c_default, I2C_BAUD);
        continue;
      }
      // error = i2c_read_timeout_us(i2c_default, slaveAddr, buf, 2*nMemAddressRead, 0, read_timeout_us);
      error = i2c_read_blocking(i2c_default, slaveAddr, buf, 2*nMemAddressRead, 0);
      if (error == PICO_ERROR_TIMEOUT) {
#ifdef DEBUG
        printf("Error: I2C bus stuck - resetting.\n");
#endif
        i2c_deinit(i2c_default);
        i2c_init(i2c_default, I2C_BAUD);
        continue;
      } else if (error < 0) {
        printf("[ERROR] I2CRead1: %d\n", error);
        i2c_deinit(i2c_default);
        i2c_init(i2c_default, I2C_BAUD);
        continue;
      }
      break;
    }

    for (int count = 0; count < nMemAddressRead; count++) {
        int i = count << 1;
        *p++ = ((uint16_t)buf[i] << 8) | buf[i + 1];
    }
    return 0;
}

void MLX90640_I2CFreqSet(int freq) {
}

int MLX90640_I2CWrite(uint8_t slaveAddr, uint16_t writeAddress, uint16_t data) {
    uint8_t cmd[4] = {0, 0, 0, 0};

    cmd[0] = writeAddress >> 8;
    cmd[1] = writeAddress & 0x00FF;
    cmd[2] = data >> 8;
    cmd[3] = data & 0x00FF;

    int error;
    // NOTE: timeout is divided by number of bytes to write/read
    uint write_timeout_us = 1000 * 4;

    for (int i = 0; i <= MAX_RETRIES; i++) {
      if (i == MAX_RETRIES) {
        return -1;
      }
      error = i2c_write_timeout_us(i2c_default, slaveAddr, cmd, 4, 0, write_timeout_us);
      if (error == PICO_ERROR_TIMEOUT) {
#ifdef DEBUG
        printf("Error: I2C bus stuck - resetting.\n");
#endif
        i2c_deinit(i2c_default);
        i2c_init(i2c_default, I2C_BAUD);
        continue;
      } else if (error < 0) {
        printf("[ERROR] I2CWrite0: %d\n", error);
        i2c_deinit(i2c_default);
        i2c_init(i2c_default, I2C_BAUD);
        continue;
      }
      break;
    }
    return 0;
}
