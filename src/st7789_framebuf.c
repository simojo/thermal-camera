/*
 * st7789_framebuf.c
 *
 * @copyright Copyright (C) 2025 Simon J. Jones <github@simonjjones.com>
 * Licensed under the Apache License, Version 2.0.
 */

#include "st7789.h"
#include <stdlib.h>
#include "fonts.h"
#include <math.h>

/*
 * @brief define the frame buffer for the st7789
 */
static uint16_t framebuf[ST7789_LINE_SIZE * ST7789_COLUMN_SIZE];
/*
 * @brief define the ram window of the frame buffer.
 */
static size_t framebuf_window_x0 = 0;
static size_t framebuf_window_y0 = 0;
static size_t framebuf_window_x1 = 0;
static size_t framebuf_window_y1 = 0;
#define FRAMEBUF_INDEX(xi, yi) yi * ST7789_LINE_SIZE + xi

void st7789_framebuf_flush(void) {
  st7789_set_window(0, 0, ST7789_LINE_SIZE-1, ST7789_COLUMN_SIZE-1);
  st7789_write_command(ST7789_CMD_RAMWR);
  st7789_write_data_words(framebuf, ST7789_LINE_SIZE * ST7789_COLUMN_SIZE);
}

inline void st7789_clip_pixel_vals(uint *_x, uint *_y) {
  // keep values within range
  if (*_x >= ST7789_LINE_SIZE) *_x = ST7789_LINE_SIZE-1;
  if (*_y >= ST7789_COLUMN_SIZE) *_y = ST7789_COLUMN_SIZE-1;
}

void st7789_framebuf_draw_pixel(uint x, uint y, uint16_t color) {
  // keep values within range
  st7789_clip_pixel_vals(&x, &y);
  framebuf[FRAMEBUF_INDEX(x, y)] = color;
}

void st7789_framebuf_set_window(size_t x0, size_t y0, size_t x1, size_t y1) {
  // keep values within range
  st7789_clip_pixel_vals(&x0, &y0);
  st7789_clip_pixel_vals(&x1, &y1);

  framebuf_window_x0 = x0;
  framebuf_window_y0 = y0;
  framebuf_window_x1 = x1;
  framebuf_window_y1 = y1;
}

void st7789_framebuf_write_data_words(uint16_t *words, size_t len) {
  size_t i = 0;
  for (size_t xi = framebuf_window_x0; xi <= framebuf_window_x1; xi++) {
    for (size_t yi = framebuf_window_y0; yi <= framebuf_window_y1; yi++) {
      if (i == len) return;
      framebuf[FRAMEBUF_INDEX(xi, yi)] = words[i++];
    }
  }
}

void st7789_framebuf_write_char(uint x0, uint y0, char c, uint16_t color, uint16_t bgcolor, bool bgtransparent) {
  // keep values within range
  st7789_clip_pixel_vals(&x0, &y0);

  st7789_framebuf_set_window(x0, y0, x0 + FONT_W - 1, y0 + FONT_H - 1);
  for (uint32_t i = 0; i < FONT_H; i++) {
    uint8_t bitmap_row = thefont[(c - 32) * FONT_H + i];
    for (uint32_t j = 0; j < FONT_W; j++) {
      // keep shifting and checking the MSb of the word
      // if it's 1, then we should draw the character's pixel
      // if it's 0, then we should draw the bgcolor
      if ((bitmap_row << j) & 0x80) {
        framebuf[FRAMEBUF_INDEX(x0 + j, y0 + i)] = color;
      }
      else {
        if (!bgtransparent) {
          framebuf[FRAMEBUF_INDEX(x0 + j, y0 + i)] = bgcolor;
        }
      }
    }
  }
}

void st7789_framebuf_write_string(uint x0, uint y0, const char *s, uint16_t color, uint16_t bgcolor, bool bgtransparent) {
  // keep values within range
  st7789_clip_pixel_vals(&x0, &y0);
  char *p = (char *)s;
  int i = 0;
  while (*p) {
    // on line breaks, increment us down a line
    if (*p == '\n') {
      y0 += FONT_H;
    }
    st7789_framebuf_write_char(x0 + i * FONT_W, y0, *p++, color, bgcolor, bgtransparent);
    i++;
  }
}

/*
 * st7789_draw_line
 *
 * @brief Draw a line on using Bresenham's line algorithm.
 */
void st7789_framebuf_draw_line(uint x0, uint y0, uint x1, uint y1, uint16_t color) {
  // keep values within range
  st7789_clip_pixel_vals(&x0, &y0);
  st7789_clip_pixel_vals(&x1, &y1);

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

/*
 * st7789_framebuf_fill_rect
 *
 * @brief Fill a rect in the framebuffer.
 */
void st7789_framebuf_fill_rect(uint x0, uint y0, uint x1, uint y1, uint16_t color) {
  // keep values within range
  st7789_clip_pixel_vals(&x0, &y0);
  st7789_clip_pixel_vals(&x1, &y1);
  // switch values if they are reversed
  if (x1 < x0) {
    uint tmp = x0;
    x0 = x1;
    x1 = tmp;
  }
  // switch values if they are reversed
  if (y1 < y0) {
    uint tmp = y0;
    y0 = y1;
    y1 = tmp;
  }
  for (size_t xi = x0; xi < x1; xi++) {
    for (size_t yi = y0; yi < y1; yi++) {
      framebuf[FRAMEBUF_INDEX(xi, yi)] = color;
    }
  }
}
