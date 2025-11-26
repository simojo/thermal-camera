#include <stdio.h>
#include <stdbool.h>
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define RGB565(R, G, B) \
    (((uint16_t)((R) & 0b11111000) << 8) | \
     ((uint16_t)((G) & 0b11111100) << 3) | \
     ((uint16_t)((B) >> 3)) )

#define RES_PIN 21
#define DC_PIN 20


bool init = false;

void st7789_init(void) {
  gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
  gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
  gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SIO);
  gpio_init(DC_PIN);
  gpio_init(RES_PIN);
  gpio_set_dir(DC_PIN, GPIO_OUT);
  gpio_set_dir(RES_PIN, GPIO_OUT);
  spi_init(spi_default, (uint)1e6);
  init = true;
}

void st7789_dc_data(void) {
  gpio_put(DC_PIN, 1);
}

void st7789_dc_command(void) {
  gpio_put(DC_PIN, 0);
}

void st7789_cs(void) {
  // chip select is active low
  gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 0);
}

void st7789_write_data(uint8_t *buf, uint len) {
  if (!init) {
    st7789_init();
  }
  st7789_dc_data();
  spi_write_blocking(spi_default, buf, len);
}

void st7789_write_command(uint8_t *buf, uint len) {
  if (!init) {
    st7789_init();
  }
  st7789_dc_command();
  spi_write_blocking(spi_default, buf, len);
}

void st7789_draw_pixel(uint x, uint y, uint16_t color) {
  // FIXME
}

void st7789_draw_line(uint x0, uint y0, uint x1, uint y1, uint16_t color) {
  // FIXME
}

void st7789_fill_rect(uint x0, uint y0, uint x1, uint y1, uint16_t color) {
  // FIXME
}

void st7789_fill_circ(uint x, uint y, uint16_t color) {
  // FIXME
}
