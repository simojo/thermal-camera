#include <stdio.h>
#include <stdbool.h>
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <malloc.h>
#include "st7789.h"
#include <math.h>
#include "mlx90640/MLX90640_API.h"

#define RES_PIN 21
#define DC_PIN 20


// command definitions (source: Shifeng Li <https://github.com/libdriver/st7789/blob/main/src/driver_st7789.c#L61-L124>)
#define ST7789_CMD_NOP             0x00        // no operation command
#define ST7789_CMD_SWRESET         0x01        // software reset command
#define ST7789_CMD_SLPIN           0x10        // sleep in command
#define ST7789_CMD_SLPOUT          0x11        // sleep out command
#define ST7789_CMD_PTLON           0x12        // partial mode on command
#define ST7789_CMD_NORON           0x13        // normal display mode on command
#define ST7789_CMD_INVOFF          0x20        // display inversion off command
#define ST7789_CMD_INVON           0x21        // display inversion on command
#define ST7789_CMD_GAMSET          0x26        // display inversion set command
#define ST7789_CMD_DISPOFF         0x28        // display off command
#define ST7789_CMD_DISPON          0x29        // display on command
#define ST7789_CMD_CASET           0x2A        // column address set command (datasheet: p 198)
#define ST7789_CMD_RASET           0x2B        // row address set command
#define ST7789_CMD_RAMWR           0x2C        // memory write command
#define ST7789_CMD_PTLAR           0x30        // partial start/end address set command
#define ST7789_CMD_VSCRDEF         0x33        // vertical scrolling definition command
#define ST7789_CMD_TEOFF           0x34        // tearing effect line off command
#define ST7789_CMD_TEON            0x35        // tearing effect line on command
#define ST7789_CMD_MADCTL          0x36        // memory data access control command
#define ST7789_CMD_VSCRSADD        0x37        // vertical scrolling start address command
#define ST7789_CMD_IDMOFF          0x38        // idle mode off command
#define ST7789_CMD_IDMON           0x39        // idle mode on command
#define ST7789_CMD_COLMOD          0x3A        // interface pixel format command
#define ST7789_CMD_RAMWRC          0x3C        // memory write continue command
#define ST7789_CMD_TESCAN          0x44        // set tear scanline command
#define ST7789_CMD_WRDISBV         0x51        // write display brightness command
#define ST7789_CMD_WRCTRLD         0x53        // write CTRL display command
#define ST7789_CMD_WRCACE          0x55        // write content adaptive brightness control and color enhancement command
#define ST7789_CMD_WRCABCMB        0x5E        // write CABC minimum brightness command
#define ST7789_CMD_RAMCTRL         0xB0        // ram control command
#define ST7789_CMD_RGBCTRL         0xB1        // rgb control command
#define ST7789_CMD_PORCTRL         0xB2        // porch control command
#define ST7789_CMD_FRCTRL1         0xB3        // frame rate control 1 command
#define ST7789_CMD_PARCTRL         0xB5        // partial mode control command
#define ST7789_CMD_GCTRL           0xB7        // gate control command
#define ST7789_CMD_GTADJ           0xB8        // gate on timing adjustment command
#define ST7789_CMD_DGMEN           0xBA        // digital gamma enable command
#define ST7789_CMD_VCOMS           0xBB        // vcoms setting command
#define ST7789_CMD_LCMCTRL         0xC0        // lcm control command
#define ST7789_CMD_IDSET           0xC1        // id setting command
#define ST7789_CMD_VDVVRHEN        0xC2        // vdv and vrh command enable command
#define ST7789_CMD_VRHS            0xC3        // vrh set command
#define ST7789_CMD_VDVSET          0xC4        // vdv setting command
#define ST7789_CMD_VCMOFSET        0xC5        // vcoms offset set command
#define ST7789_CMD_FRCTR2          0xC6        // fr control 2 command
#define ST7789_CMD_CABCCTRL        0xC7        // cabc control command
#define ST7789_CMD_REGSEL1         0xC8        // register value selection1 command
#define ST7789_CMD_REGSEL2         0xCA        // register value selection2 command
#define ST7789_CMD_PWMFRSEL        0xCC        // pwm frequency selection command
#define ST7789_CMD_PWCTRL1         0xD0        // power control 1 command
#define ST7789_CMD_VAPVANEN        0xD2        // enable vap/van signal output command
#define ST7789_CMD_CMD2EN          0xDF        // command 2 enable command
#define ST7789_CMD_PVGAMCTRL       0xE0        // positive voltage gamma control command
#define ST7789_CMD_NVGAMCTRL       0xE1        // negative voltage gamma control command
#define ST7789_CMD_DGMLUTR         0xE2        // digital gamma look-up table for red command
#define ST7789_CMD_DGMLUTB         0xE3        // digital gamma look-up table for blue command
#define ST7789_CMD_GATECTRL        0xE4        // gate control command
#define ST7789_CMD_SPI2EN          0xE7        // spi2 command
#define ST7789_CMD_PWCTRL2         0xE8        // power control 2 command
#define ST7789_CMD_EQCTRL          0xE9        // equalize time control command
#define ST7789_CMD_PROMCTRL        0xEC        // program control command
#define ST7789_CMD_PROMEN          0xFA        // program mode enable command
#define ST7789_CMD_NVMSET          0xFC        // nvm setting command
#define ST7789_CMD_PROMACT         0xFE        // program action command

volatile bool st7789_is_init = false;

/*
 * define ten colors that create the heat map
 */
const uint16_t heatmap_color_rgb565[] = {
  RGB565(0,   0,   128), // dark blue
  RGB565(0,   0,   255), // blue
  RGB565(0,   128, 255), // cyan
  RGB565(0,   255, 128), // turquoise
  RGB565(0,   255, 0),   // green
  RGB565(128, 255, 0),   // yellow-green
  RGB565(255, 255, 0),   // yellow
  RGB565(255, 128, 0),   // orange
  RGB565(255, 64,  0),   // red-orange
  RGB565(255, 255, 255), // white (hottest)
};

void st7789_write_data_byte(uint8_t b);
void st7789_write_command(uint8_t cmd);

void st7789_init(void) {
  printf("here\n");
  gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
  gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
  gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);

  gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
  gpio_init(DC_PIN);
  gpio_init(RES_PIN);
  gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
  gpio_set_dir(DC_PIN, GPIO_OUT);
  gpio_set_dir(RES_PIN, GPIO_OUT);
  spi_init(spi_default, (uint)20e6);

  gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
  gpio_put(DC_PIN, 1);

  gpio_put(RES_PIN, 0);
  sleep_ms(20);
  gpio_put(RES_PIN, 1);
  sleep_ms(150);

  st7789_is_init = true;

  // heavily inspired from adafruit's source code for arduino with the st7789
  // https://github.com/adafruit/Adafruit-ST7735-Library/blob/master/Adafruit_ST7789.cpp#L50-L77
  st7789_write_command(ST7789_CMD_SWRESET);
  sleep_ms(150);
  st7789_write_command(ST7789_CMD_SLPOUT);
  sleep_ms(10);
  st7789_write_command(ST7789_CMD_COLMOD);
  st7789_write_data_byte(0x55); // 65k RGB interface, 16b per pixel (datasheet: p 224)
  sleep_ms(10);
  st7789_write_command(ST7789_CMD_MADCTL);
  st7789_write_data_byte(0x60); // MV, MX, MY = 1 1 0 (rotate 90 clockwise)
  st7789_write_command(ST7789_CMD_INVON); // adafruit claims this helps to avoid startup issues
  sleep_ms(10);
  st7789_write_command(ST7789_CMD_NORON); // normal display mode on
  sleep_ms(10);
  st7789_write_command(ST7789_CMD_DISPON); // display on
  sleep_ms(10);
}

void st7789_dc_data(void) {
  gpio_put(DC_PIN, 1);
}

void st7789_dc_command(void) {
  gpio_put(DC_PIN, 0);
}

void st7789_select(void) {
  // chip select is active low
  gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 0);
}

void st7789_unselect(void) {
  // chip select is active low (make high)
  gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
}

void st7789_write_data(uint8_t *buf, uint len) {
  if (!st7789_is_init) {
    st7789_init();
  }
  st7789_dc_data();
  st7789_select();
  spi_write_blocking(spi_default, buf, len);
  st7789_unselect();
}

void st7789_write_data_byte(uint8_t b) {
  st7789_write_data(&b, 1);
}


void st7789_write_command(uint8_t cmd) {
  if (!st7789_is_init) {
    st7789_init();
  }
  st7789_dc_command();
  st7789_select();
  spi_write_blocking(spi_default, &cmd, 1);
  st7789_unselect();
}

void st7789_set_window(uint x0, uint x1, uint y0, uint y1) {
  uint8_t data[4];

  // column address set for x0, x1
  st7789_write_command(ST7789_CMD_CASET);
  data[0] = (x0 >> 8) & 0xFF;
  data[1] = (x0 >> 0) & 0xFF;
  data[2] = (x1 >> 8) & 0xFF;
  data[3] = (x1 >> 0) & 0xFF;
  st7789_write_data(data, 4);

  // row address set for y0, y1
  st7789_write_command(ST7789_CMD_RASET);
  data[0] = (y0 >> 8) & 0xFF;
  data[1] = (y0 >> 0) & 0xFF;
  data[2] = (y1 >> 8) & 0xFF;
  data[3] = (y1 >> 0) & 0xFF;
  st7789_write_data(data, 4);
}

void st7789_draw_pixel(uint x, uint y, uint16_t color) {
  st7789_set_window(x, x, y, y);
  st7789_write_command(ST7789_CMD_RAMWR);
  uint8_t data[2];

  data[0] = (color >> 8) & 0xFF; // HSB
  data[1] = (color >> 0) & 0xFF; // LSB

  st7789_write_data(data, 2);
}

void st7789_draw_line(uint x0, uint y0, uint x1, uint y1, uint16_t color) {
  // FIXME
}

void st7789_fill_rect(uint x0, uint y0, uint x1, uint y1, uint16_t color) {
  st7789_set_window(x0, x1, y0, y1);
  st7789_write_command(ST7789_CMD_RAMWR);
  uint32_t rect_size_words = (x1 - x0 + 1) * (y1 - y0 + 1);
  uint32_t rect_size_bytes = rect_size_words * sizeof(uint16_t);
  // each pixel has size uint16_t
  uint8_t *buf = malloc(rect_size_bytes);
  if (!buf) {
    printf("[ERROR] st7789_fill_rect: unable to malloc.\n");
    return;
  }
  // fill the entire buffer with the same color
  for (int i = 0; i < rect_size_words; i++) {
    buf[2 * i]     = (color >> 8) & 0xFF; // HSB
    buf[2 * i + 1] = (color >> 0) & 0xFF; // LSB
  }
  st7789_write_data(buf, rect_size_bytes);
  free(buf);
}

void st7789_fill_32_24(float *frame) {
  // calculate min and max temperature values
  float max_temp = -INFINITY;
  float min_temp = INFINITY;
  for (int i = 0; i < MLX90640_PIXEL_NUM; i++) {
    if (frame[i] > max_temp) {
      max_temp = frame[i];
    }
    if (frame[i] < min_temp) {
      min_temp = frame[i];
    }
  }

  for (int xi = 0; xi < 32; xi++) {
    for (int yi = 0; yi < 24; yi++) {
      // FIXME: find way to write frame correctly
      uint16_t color_i = heatmap_color_rgb565[(uint)((frame[yi * 32 + xi] - min_temp) / (max_temp - min_temp) * 10)];
      st7789_fill_rect(10 * xi, 10 * yi, 10 * xi + 9, 10 * yi + 9, color_i);
    }
  }
}

void st7789_fill_circ(uint x, uint y, uint r, uint16_t color) {
  // FIXME
}
