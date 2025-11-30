#ifndef _ST7789_FRAMEBUF_H
#define _ST7789_FRAMEBUF_H

#include <stdint.h>
#include <stdlib.h>

/*
 * st7789_framebuf_flush
 *
 * @brief Flush the frame buffer by writing to the st7789.
 */
void st7789_framebuf_flush(void);
/*
 * st7789_framebuf_draw_pixel
 *
 * @brief Draw a single pixel to the frame buffer.
 */
void st7789_framebuf_draw_pixel(uint x, uint y, uint16_t color);
/*
 * st7789_framebuf_set_window
 *
 * @brief Set the window of the frame buffer. Intended to be a drop-in replacement for st7789_set_window.
 */
void st7789_framebuf_set_window(size_t x0, size_t y0, size_t x1, size_t y1);
/*
 * st7789_framebuf_write_data_words
 *
 * @brief Set the window of the frame buffer. Intended to be a drop-in replacement for st7789_set_window.
 */
void st7789_framebuf_write_data_words(uint16_t *words, size_t len);
/*
 * st7789_framebuf_write_char
 *
 * @brief Write a char to the frame buffer at the given location.
 */
void st7789_framebuf_write_char(uint x0, uint y0, char c, uint16_t color, uint16_t bgcolor, bool bgtransparent);
/*
 * st7789_framebuf_write_string
 *
 * @brief Write a string to the frame buffer at the given location.
 */
void st7789_framebuf_write_string(uint x0, uint y0, const char *s, uint16_t color, uint16_t bgcolor, bool bgtransparent);
/*
 * st7789_draw_line
 *
 * @brief Draw a line on using Bresenham's line algorithm.
 */
void st7789_framebuf_draw_line(uint x0, uint y0, uint x1, uint y1, uint16_t color);
/*
 * st7789_framebuf_fill_rect
 *
 * @brief Fill a rect in the framebuffer.
 */
void st7789_framebuf_fill_rect(uint x0, uint y0, uint x1, uint y1, uint16_t color);

#endif
