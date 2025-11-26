#include <stdio.h>
#include <stdbool.h>
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define RES_PIN 21
#define DC_PIN 20

bool init = false;

void st7789_init(void) {
  gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
  gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
  gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI);
  gpio_init(DC_PIN);
  gpio_init(RES_PIN);
  gpio_set_dir(DC_PIN, GPIO_OUT);
  gpio_set_dir(RES_PIN, GPIO_OUT);
  spi_init(PICO_DEFAULT_SPI_INSTANCE, (uint)1e6);
  init = true;
}

void st7789_dc_data(void) {
  gpio_put(DC_PIN, 1);
}

void st7789_dc_command(void) {
  gpio_put(DC_PIN, 0);
}

void st7789_write() {
  if (!init) {
    st7789_init();
  }
}
