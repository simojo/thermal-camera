#ifndef _ST7789_H
#define _ST7789_H

#include <stdint.h>
#include "pico/stdlib.h"

#define ST7789_LINE_SIZE 320
#define ST7789_COLUMN_SIZE 240

// quick convert for three RGB values to rgb-565
#define RGB565(R, G, B) \
  (((uint16_t)((R) & 0b11111000) << 8) | \
    ((uint16_t)((G) & 0b11111100) << 3) | \
    ((uint16_t)((B) >> 3)))


#define BLACK RGB565(0,   0,   0)
#define WHITE RGB565(255, 255, 255)
#define RED   RGB565(255, 0,   0)
#define BLUE  RGB565(0,   0,   255)
//
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

/*
 * st7789_init
 *
 * @brief Initialize the screen.
 */
void st7789_init(void);
/*
 * st7789_write_data_words
 *
 * @brief Mutate buffer into big-endian and send to st7789_write_data.
 */
void st7789_write_data_words(uint16_t *buf, uint len);
/*
 * st7789_write_command
 *
 * @brief Write a command (as opposed to data) to the st7789.
 */
void st7789_write_command(uint8_t cmd);
/*
 * st7789_draw_pixel
 *
 * @brief Draw a pixel at the specified coordinates with the given color.
 */
void st7789_draw_pixel(uint x, uint y, uint16_t color);
/*
 * st7789_set_window
 *
 * @brief Set the window where the pixel data will be drawn to.
 */
void st7789_set_window(uint x0, uint y0, uint x1, uint y1);
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
