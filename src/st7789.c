/*
 * st7789.c
 *
 * @copyright Copyright (C) 2025 Simon J. Jones <github@simonjjones.com>
 * Licensed under the Apache License, Version 2.0.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <malloc.h>
#include "st7789.h"
#include "fonts.h"
#include "st7789_framebuf.h"
#include <math.h>
#include "mlx90640/MLX90640_API.h"

#define RES_PIN 21
#define DC_PIN 20

volatile bool st7789_is_init = false;

#define N_HEATMAP_COLORS 20
/*
 * define ten colors that create the heat map
 */
const uint16_t heatmap_color_rgb565[] = {
    RGB565(0, 0, 128),
    RGB565(0, 0, 191),
    RGB565(0, 64, 255),
    RGB565(0, 128, 255),
    RGB565(0, 191, 255),
    RGB565(0, 255, 255),
    RGB565(0, 255, 191),
    RGB565(0, 255, 128),
    RGB565(0, 255, 64),
    RGB565(0, 255, 0),
    RGB565(64, 255, 0),
    RGB565(128, 255, 0),
    RGB565(191, 255, 0),
    RGB565(255, 255, 0),
    RGB565(255, 191, 0),
    RGB565(255, 128, 0),
    RGB565(255, 64, 0),
    RGB565(255, 0, 0),
    RGB565(191, 0, 0),
    RGB565(128, 0, 0)
};



void st7789_write_data_byte(uint8_t b);
void st7789_write_command(uint8_t cmd);

void st7789_init(void) {
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

void st7789_write_data_words(uint16_t *buf, uint len) {
  size_t newlen = len * 2;

  // shift word-sized buffer around to be big-endian
  for (int i = 0; i < len; i++) {
    uint16_t temp = buf[i];
    buf[i] = (temp >> 8) | (temp << 8); // HSB -> LSB order
  }
  // cast buffer as uint8_t* after we shift its endianness
  st7789_write_data((uint8_t*)buf, newlen);

  // unshift word-sized buffer around to be little-endian
  // so that we don't leave the data mutated.
  for (int i = 0; i < len; i++) {
    uint16_t temp = buf[i];
    buf[i] = (temp >> 8) | (temp << 8);
  }
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

void st7789_set_window(uint x0, uint y0, uint x1, uint y1) {
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
  st7789_set_window(x, y, x, y);
  st7789_write_command(ST7789_CMD_RAMWR);
  uint8_t data[2];

  data[0] = (color >> 8) & 0xFF; // HSB
  data[1] = (color >> 0) & 0xFF; // LSB

  st7789_write_data(data, 2);
}

/*
 * st7789_draw_line
 *
 * @brief Draw a line on using Bresenham's line algorithm.
 */
void st7789_draw_line(uint x0, uint y0, uint x1, uint y1, uint16_t color) {
  // use Bresenham's line algorithm (source: https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm)

  int dx = abs((int)x1 - (int)x0);
  // direction of x
  int dirx = x0 < x1 ? 1 : -1;
  int dy = -abs((int)y1 - (int)y0);
  // direction of x
  int diry = y0 < y1 ? 1 : -1;
  int e = dx + dy;

  // modify x0,y0 directly because we passed by value
  while (1) {
    st7789_draw_pixel(x0, y0, color);

    if (x0 == x1 && y0 == y1) {
      break;
    }

    int e2 = 2 * e;
    if (e2 >= dy) {
      e += dy;
      x0 += dirx;
    }
    if (e2 <= dx) {
      e += dx;
      y0 += diry;
    }
  }
}

void st7789_fill_rect(uint x0, uint y0, uint x1, uint y1, uint16_t color) {
  st7789_set_window(x0, y0, x1, y1);
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

static uint8_t st7789_loading_ani_state = 0;
#define ST7789_LOADING_ANI_N_STATES 7
#define ST7789_LOADING_ANI_N_LINES 6
#define ST7789_LOADING_ANI_N_CHARS 13
static const char st7789_loading_ani_words[ST7789_LOADING_ANI_N_STATES][ST7789_LOADING_ANI_N_CHARS+1] = {
  "   Loading   ",
  "  <Loading>  ",
  " <<Loading>> ",
  "<<<Loading>>>",
  " <<Loading>> ",
  "  <Loading>  ",
  "   Loading   ",
};
void st7789_loading_ani_tick(void) {
  size_t center_x = ST7789_LINE_SIZE / 2;
  size_t center_y = ST7789_COLUMN_SIZE / 2;
  size_t r0 = 10;
  size_t r1 = 30;

  // first clear the framebuf
  st7789_framebuf_fill_rect(0, 0, ST7789_LINE_SIZE-1, ST7789_COLUMN_SIZE-1, BLACK);

  // place "<<Loading>>" in the very center of the screen
  st7789_framebuf_write_string(
    ST7789_LINE_SIZE/2 - ST7789_LOADING_ANI_N_CHARS/2 * FONT_W,
    FONT_H * 2,
    st7789_loading_ani_words[st7789_loading_ani_state % ST7789_LOADING_ANI_N_STATES],
    ST7789_LOADING_ANI_N_CHARS,
    WHITE,
    BLACK,
    true
  );

  st7789_framebuf_write_string(
    ST7789_LINE_SIZE/2 - 11 * FONT_W,
    ST7789_COLUMN_SIZE-3*FONT_H,
    "Made by Simon J. Jones",
    22,
    WHITE,
    BLACK,
    true
  );
  st7789_framebuf_write_string(
    ST7789_LINE_SIZE/2 - 4 * FONT_W,
    ST7789_COLUMN_SIZE-2*FONT_H,
    "(C) 2025",
    8,
    WHITE,
    BLACK,
    true
  );

  float theta = (float)st7789_loading_ani_state / ST7789_LOADING_ANI_N_STATES * (M_PI / 3.0f);
  for (int i = 0; i < ST7789_LOADING_ANI_N_STATES; i++) {
    float theta_shift = (float)i / ST7789_LOADING_ANI_N_LINES * 2 * M_PI;
    st7789_framebuf_draw_line(
      center_x + r0 * cos(theta + theta_shift),
      center_y + r0 * sin(theta + theta_shift),
      center_x + r1 * cos(theta + theta_shift),
      center_y + r1 * sin(theta + theta_shift),
      WHITE
    );
  }

  st7789_loading_ani_state = (st7789_loading_ani_state + 1) % ST7789_LOADING_ANI_N_STATES;

  st7789_framebuf_flush();
}

/*
 * MLX90640 screen orienation
 *
 *       31 30          0
 *       l  l           l
 *       o  o    ...    o
 *       c  c           c
 * row0  ----------------
 * row1  |A             |
 *       |              |
 * ...   |              |
 *       |             B|
 * row23 ----------------
 *
 */

/*
 * @brief define where the MLX90640 A, B markers get placed in the ST7789's screen
 */
#define MLX90640_A_GLOBAL_X (ST7789_LINE_SIZE-1)
#define MLX90640_A_GLOBAL_Y (0)
#define MLX90640_B_GLOBAL_X (ST7789_LINE_SIZE/2-30)
#define MLX90640_B_GLOBAL_Y (ST7789_COLUMN_SIZE-1)

/*
 * @brief In MLX90640 perspective, define (x,y) = (0,0) to be the "A" corner of
 * the camera; define (x,y) = (max,max) to be "B" corner
 *
 * @note I don't like how they (Melexis) have 0-index on the right. It just leads to confusion.
 */
void mlx90640_xy_to_st7789_xy(size_t xi_mlx, size_t yi_mlx, size_t *xi_st, size_t *yi_st) {
  if (false) {
  } else if (MLX90640_A_GLOBAL_X < MLX90640_B_GLOBAL_X && MLX90640_A_GLOBAL_Y < MLX90640_B_GLOBAL_Y) {
    // A at top-left, B at bottom-right (0deg CCW)
    float sx = (float)((int)MLX90640_B_GLOBAL_X - (int)MLX90640_A_GLOBAL_X) / (MLX90640_LINE_SIZE - 1);
    float sy = (float)((int)MLX90640_B_GLOBAL_Y - (int)MLX90640_A_GLOBAL_Y) / (MLX90640_COLUMN_SIZE - 1);
    *xi_st = (size_t)(MLX90640_A_GLOBAL_X + xi_mlx * sx + 0.5f);
    *yi_st = (size_t)(MLX90640_A_GLOBAL_Y + yi_mlx * sy + 0.5f);
  } else if (MLX90640_A_GLOBAL_X < MLX90640_B_GLOBAL_X && MLX90640_A_GLOBAL_Y > MLX90640_B_GLOBAL_Y) {
    // A at bottom-left, B at top-right (90deg CCW)
    float sx = (float)((int)MLX90640_B_GLOBAL_X - (int)MLX90640_A_GLOBAL_X) / (MLX90640_COLUMN_SIZE - 1);
    float sy = (float)((int)MLX90640_B_GLOBAL_Y - (int)MLX90640_A_GLOBAL_Y) / (MLX90640_LINE_SIZE - 1);
    *xi_st = (size_t)(MLX90640_A_GLOBAL_X + yi_mlx * sx + 0.5f);
    *yi_st = (size_t)(MLX90640_A_GLOBAL_Y + xi_mlx * sy + 0.5f);
  } else if (MLX90640_A_GLOBAL_X > MLX90640_B_GLOBAL_X && MLX90640_A_GLOBAL_Y > MLX90640_B_GLOBAL_Y) {
    // A at bottom-right, B at top-left (180deg CCW)
    float sx = (float)((int)MLX90640_B_GLOBAL_X - (int)MLX90640_A_GLOBAL_X) / (MLX90640_LINE_SIZE - 1);
    float sy = (float)((int)MLX90640_B_GLOBAL_Y - (int)MLX90640_A_GLOBAL_Y) / (MLX90640_COLUMN_SIZE - 1);
    *xi_st = (size_t)(MLX90640_A_GLOBAL_X + xi_mlx * sx + 0.5f);
    *yi_st = (size_t)(MLX90640_A_GLOBAL_Y + yi_mlx * sy + 0.5f);
  } else if (MLX90640_A_GLOBAL_X > MLX90640_B_GLOBAL_X && MLX90640_A_GLOBAL_Y < MLX90640_B_GLOBAL_Y) {
    // A at top-right, B at bottom-left (270deg CCW)
    float sx = (float)((int)MLX90640_B_GLOBAL_X - (int)MLX90640_A_GLOBAL_X) / (MLX90640_COLUMN_SIZE - 1);
    float sy = (float)((int)MLX90640_B_GLOBAL_Y - (int)MLX90640_A_GLOBAL_Y) / (MLX90640_LINE_SIZE - 1);
    *xi_st = (size_t)(MLX90640_A_GLOBAL_X + yi_mlx * sx + 0.5f);
    *yi_st = (size_t)(MLX90640_A_GLOBAL_Y + xi_mlx * sy + 0.5f);
  }
}

// define the index to go in the opposite x-order, because the MLX90640 goes right->left, top-bottom
#define MLX90640_INDEX(xi, yi) ((yi) * MLX90640_LINE_SIZE + (MLX90640_LINE_SIZE - 1 - (xi)))

uint32_t st7789_fill_32_24_fps_estimate_t0 = 0;
uint32_t st7789_fill_32_24_fps_estimate_t1 = 0;

void st7789_fill_32_24(float *frame) {
  /*
   * %%%%%%%%%%%%%%
   * FPS estimation
   * %%%%%%%%%%%%%%
   */
  st7789_fill_32_24_fps_estimate_t1 = time_us_32() / 1000;
  char fps_buf[32];
  float fps_estimate = 1000.0f/(st7789_fill_32_24_fps_estimate_t1 - st7789_fill_32_24_fps_estimate_t0);
  snprintf(fps_buf, 32, "FPS: %3.2f", fps_estimate);
  st7789_framebuf_write_string(10, 10, fps_buf, 32, WHITE, BLACK, false);
  st7789_fill_32_24_fps_estimate_t0 = st7789_fill_32_24_fps_estimate_t1;

  /*
   * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
   * calculate min and max temperature values
   * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
   */
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
  char temp_buf[12];
  snprintf(temp_buf, 11+1, "Max: % 5.2f", max_temp);
  st7789_framebuf_write_string(10, ST7789_COLUMN_SIZE/2-FONT_H, temp_buf, 11+1, WHITE, heatmap_color_rgb565[N_HEATMAP_COLORS-1], false);
  snprintf(temp_buf, 11+1, "Min: % 5.2f", min_temp);
  st7789_framebuf_write_string(10, ST7789_COLUMN_SIZE/2, temp_buf, 11+1, WHITE, heatmap_color_rgb565[0], false);

  /*
   * %%%%%%%%%%%%%%%%%%%%%%%%%
   * temperature color mapping
   * %%%%%%%%%%%%%%%%%%%%%%%%%
   */
  for (size_t mlx90640_yi = 0; mlx90640_yi < (MLX90640_COLUMN_SIZE-1); mlx90640_yi++) {
    for (size_t mlx90640_xi = 0; mlx90640_xi < (MLX90640_LINE_SIZE-1); mlx90640_xi++) {
      // calcualte heatmap color index based on range of mlx90640 color values in frame
      float pix = frame[MLX90640_INDEX(mlx90640_xi,mlx90640_yi)];
      size_t heatmap_color_rgb565_ind = (size_t)((pix - min_temp) / (max_temp - min_temp) * (N_HEATMAP_COLORS - 1));

      // fetch newly calculated color from heatmap array
      uint16_t heatmap_color_rgb565_value = heatmap_color_rgb565[heatmap_color_rgb565_ind];

      // grab coordinates of window in st7789 space
      size_t st7789_x0 = 0;
      size_t st7789_y0 = 0;
      mlx90640_xy_to_st7789_xy(mlx90640_xi, mlx90640_yi, &st7789_x0, &st7789_y0);
      size_t st7789_x1 = 0;
      size_t st7789_y1 = 0;
      mlx90640_xy_to_st7789_xy(mlx90640_xi+1, mlx90640_yi+1, &st7789_x1, &st7789_y1);

      // update st7789 framebuffer with newly calculated color
      st7789_framebuf_fill_rect(
        st7789_x0,
        st7789_y0,
        st7789_x1,
        st7789_y1,
        heatmap_color_rgb565_value
      );
    }
  }

  // now that we've done all of our transformations, flush the frame buffer
  st7789_framebuf_flush();
}

void st7789_fill_circ(uint x, uint y, uint r, uint16_t color) {
  // draw center first, this avoids division by 0
  st7789_framebuf_draw_pixel(x, y, color);
  // draw a bunch of concentric circles incrementing by dtheta(r) = 1/r, which
  // is to maintain approximately 1 pixel arc length along the circle
  for (size_t ri = 1; ri <= r; ri++) {
    for (float theta = 0.0f; theta <= 2 * M_PI; theta += 1.0f/((float)r)) {
      st7789_framebuf_draw_pixel(x + (int)((float)ri * cos(theta)), y + (int)((float)ri * sin(theta)), color);
    }
  }
  st7789_framebuf_flush();
}
