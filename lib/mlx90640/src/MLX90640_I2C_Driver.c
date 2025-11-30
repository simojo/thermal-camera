/**
 * @copyright (C) 2025 Simon J. Jones <github@simonjjones.com>
 */

#include <stddef.h>
#include <stdio.h>

#include "mlx90640/MLX90640_I2C_Driver.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

/*
 * The EEPROM is limited to a 400kHz baud rate. After we're done with it, we can go 1 Mhz.
 */
#define I2C_BAUD 400 * 1000

bool init = 0;

#define SDA 4
#define SCL 5

void MLX90640_I2CInit() {
}

static inline void sclh(void) {
  gpio_set_dir(SCL, GPIO_IN);
}

static inline void scll(void) {
  gpio_set_dir(SCL, GPIO_OUT);
  gpio_put(SCL, 0);
}

static inline uint8_t sda_read(void) {
  return (uint8_t)gpio_get(SDA);
}

static inline void sdah(void) {
  gpio_set_dir(SDA, GPIO_IN);
}

static inline void sdal(void) {
  gpio_set_dir(SDA, GPIO_OUT);
  gpio_put(SDA, 0);
}

int _i2c_write_blocking(uint8_t slaveAddr, uint8_t *buf, uint16_t nbytes, bool nostop) {
  // start condition
  sclh();
  sdal();
  scll();

  uint8_t ack = 1;

  // slaveAddr is 7 bits, so we shift by 1 to start at 7th bit
  uint8_t addr_byte_msk = 0x80;
  slaveAddr = (slaveAddr << 1) | 0; // write LSB is LOW
  for (int j = 0; j < 8; j++) {
    slaveAddr & addr_byte_msk ? sdah() : sdal();
    sclh();
    addr_byte_msk >>= 1;
    scll();
  }
  sdah();
  sclh();
  ack = sda_read();
  scll();
  if (ack == 1) {
    // stop condition
    sdal();
    sclh();
    sdah();
    return -1;
  }

  uint8_t *p = buf;
  for (int i = 0; i < nbytes; i++) {
    ack = 1;
    uint8_t byte_msk = 0x80;
    for (int j = 0; j < 8; j++) {
      *p & byte_msk ? sdah() : sdal();
      sclh();
      byte_msk >>= 1;
      scll();
    }
    sdah();
    sclh();
    ack = sda_read();
    scll();
    if (ack == 0) {
      p++;
      continue;
    } else {
      // stop condition
      sdal();
      sclh();
      sdah();
      return -1;
    }
  }
  if (!nostop) {
    // stop condition
    sdal();
    sclh();
    sdah();
  }
  return 0;
}

int _i2c_read_blocking(uint8_t slaveAddr, uint8_t *buf, uint16_t nbytes, bool nostop) {
  // start condition
  sclh();
  sdal();
  scll();

  uint8_t ack = 1;

  uint8_t addr_byte_msk = 0x80;
  slaveAddr = (slaveAddr << 1) | 1; // write LSB is LOW
  for (int j = 0; j < 8; j++) {
    slaveAddr & addr_byte_msk ? sdah() : sdal();
    sclh();
    addr_byte_msk >>= 1;
    scll();
  }
  sdah();
  sclh();
  ack = sda_read();
  scll();
  if (ack == 1) {
    // stop condition
    sdal();
    sclh();
    sdah();
    return -1;
  }


  uint8_t *p = buf;
  for (int i = 0; i < nbytes; i++) {
    ack = 1;
    for (int j = 0; j < 8; j++) {
      sclh();
      *p = (*p << 1) | sda_read();
      scll();
    }
    sdal();
    sclh();
    p++;
    scll();
    sdah();
  }
  if (!nostop) {
    // stop condition
    sdal();
    sclh();
    sdah();
  }
  return 0;
}

int MLX90640_I2CGeneralReset() {
  return 0;
}

static inline void _i2c_init(void) {
  init = true;
  i2c_init(i2c0, I2C_BAUD);
  gpio_set_function(SDA, GPIO_FUNC_I2C);
  gpio_set_function(SCL, GPIO_FUNC_I2C);
  /*
  FIXME: for bit banging
  gpio_init(SDA);
  gpio_init(SCL);
  gpio_set_function(SDA, GPIO_FUNC_SIO);
  gpio_set_function(SCL, GPIO_FUNC_SIO);
  gpio_set_dir(SDA, GPIO_IN);
  gpio_set_dir(SCL, GPIO_IN);
  */
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

    int error = 0;

    // NOTE: bit banging implementation
    // error |= _i2c_write_blocking(slaveAddr, cmd, 2, 1);
    // error |= _i2c_read_blocking(slaveAddr, buf, 2*nMemAddressRead, 0);
    if (i2c_write_blocking(i2c0, slaveAddr, cmd, 2, 1) < 0) {
      error = -1;
      return error;
    }
    if (i2c_read_blocking(i2c0, slaveAddr, buf, 2*nMemAddressRead, 0) < 0) {
      error = -1;
      return error;
    }

    for (int count = 0; count < nMemAddressRead; count++) {
        int i = count << 1;
        *p++ = ((uint16_t)buf[i] << 8) | buf[i + 1];
    }
    return 0;
}

void MLX90640_I2CFreqSet(int freq) {
  i2c_set_baudrate(i2c0, freq);
}

int MLX90640_I2CWrite(uint8_t slaveAddr, uint16_t writeAddress, uint16_t data) {
    uint8_t cmd[4] = {0, 0, 0, 0};

    cmd[0] = writeAddress >> 8;
    cmd[1] = writeAddress & 0x00FF;
    cmd[2] = data >> 8;
    cmd[3] = data & 0x00FF;

    int error = 0;

    // NOTE: bit banging implementation
    // error |= _i2c_write_blocking(slaveAddr, cmd, 4, 0);
    if (i2c_write_blocking(i2c0, slaveAddr, cmd, 4, 0) < 0) {
      error = -1;
      return error;
    }
    return error;
}
