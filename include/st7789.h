#ifndef _ST7789_H
#define _ST7789_H

#include <stdint.h>
#include "pico/stdlib.h"

// quick convert for three RGB values to rgb-565
#define RGB565(R, G, B) \
  (((uint16_t)((R) & 0b11111000) << 8) | \
    ((uint16_t)((G) & 0b11111100) << 3) | \
    ((uint16_t)((B) >> 3)))


#define BLACK RGB565(0,   0,   0)
#define WHITE RGB565(255, 255, 255)
#define RED   RGB565(255, 0,   0)
#define BLUE  RGB565(0,   0,   255)

/*
 * st7789_init
 *
 * @brief Initialize the screen.
 */
void st7789_init(void);
/*
 * st7789_draw_pixel
 *
 * @brief Draw a pixel at the specified coordinates with the given color.
 */
void st7789_draw_pixel(uint x, uint y, uint16_t color);
/*
 * st7789_draw_line
 *
 * @brief Draw a line starting from (x0,y0) ending at (x1,y1) with the given color.
 */
void st7789_draw_line(uint x0, uint y0, uint x1, uint y1, uint16_t color);
/*
 * st7789_fill_rect
 *
 * @brief Fill a rectangle with corners at (x0,y0) and (x1,y1) with the given color.
 */
void st7789_fill_rect(uint x0, uint y0, uint x1, uint y1, uint16_t color);
/*
 * st7789_fill_32_24
 *
 * @brief Fill the entire screen given frame data from the MLX90640.
 */
void st7789_fill_32_24(float *frame);
/*
 * st7789_fill_circ
 *
 * @brief Fill a circle with origin at (x,y), radius r, and the given color.
 */
void st7789_fill_circ(uint x, uint y, uint r, uint16_t color);

#endif
