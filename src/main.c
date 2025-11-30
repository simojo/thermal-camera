#include <pico/stdio.h>
#include <math.h>
#include <stdio.h>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "mlx90640/MLX90640_API.h"
#include "mlx90640/MLX90640_I2C_Driver.h"
#include "st7789.h"
#include "st7789_framebuf.h"
#include "pico/multicore.h"
#include "pico/mutex.h"

#define MLX90640_ADDR 0x33

const char *heatmap_ascii = " .:-=+*#%@";
const uint16_t heatmap_color[] = {
    RGB565(20,  20,  60),
    RGB565(45,  35,  90),
    RGB565(70,  50,  120),
    RGB565(110, 75,  140),
    RGB565(155, 100, 130),
    RGB565(200, 125, 100),
    RGB565(230, 160, 75),
    RGB565(245, 190, 90),
    RGB565(255, 220, 140),
    RGB565(255, 250, 200),
};

static uint16_t eeData[MLX90640_EEPROM_DUMP_NUM];

paramsMLX90640 mlx90640;
static float frameTemperatureCore0[MLX90640_PIXEL_NUM];
static float frameTemperatureCore1[MLX90640_PIXEL_NUM];
static uint16_t frameData[MLX90640_PIXEL_NUM + 64 + 2];

mutex_t mutex;
volatile bool frameTemperatureChanged = false;

/*
 * core1_main
 *
 * @brief This core is for loading and sending the frame buffer.
 */
void core1_main() {
  // clear the st7789 display
  st7789_init();
  st7789_framebuf_fill_rect(0, 0, 319, 239, BLACK);
  st7789_framebuf_flush();

  while (1) {
    mutex_enter_blocking(&mutex);
    if (frameTemperatureChanged) {
      st7789_fill_32_24(frameTemperatureCore1);
    }
    // critical section
    mutex_exit(&mutex);
  }
}

int main() {
  stdio_init_all();

  // initialize the mutex and launch core for handling st7789
  mutex_init(&mutex);
  multicore_launch_core1(core1_main);

  for (int i = 0; i < 3; i++) {
    printf("\rO o o o");
    sleep_ms(500);
    printf("\ro O o o");
    sleep_ms(500);
    printf("\ro o O o");
    sleep_ms(500);
    printf("\ro o o O");
    sleep_ms(500);
  }
  printf("\r\n");

  printf("[INFO] set refresh rate.\n");
  MLX90640_SetChessMode(MLX90640_ADDR);
  printf("[INFO] set chess mode.\n");
  MLX90640_SetSubPageRepeat(MLX90640_ADDR, 0);
  printf("[INFO] set subpage repeat.\n");

  if (MLX90640_DumpEE(MLX90640_ADDR, eeData) != 0) {
    printf("[ERROR] DumpEE returned error.\n");
  }
  printf("[INFO] dumpEE.\n");
  MLX90640_SetRefreshRate(MLX90640_ADDR, 0b011);

  // after reading the EEPROM, we can read much faster.
  MLX90640_I2CFreqSet(1000 * 1000);

  if (MLX90640_ExtractParameters(eeData, &mlx90640) != 0) {
    printf("[ERROR] ExtractParameters returned error.\n");
  }
  printf("[INFO] extractparameters.\n");
  MLX90640_I2CWrite(MLX90640_ADDR, MLX90640_STATUS_REG, MLX90640_INIT_STATUS_VALUE);

  uint16_t frameTemperatureColorsRGB565[MLX90640_PIXEL_NUM];

  while (1) {
    int subpage = 0;
    subpage = MLX90640_GetFrameData(MLX90640_ADDR, frameData);
    // 0, 1 are valid subpge return values. anything else implies error
    if (subpage == 0 || subpage == 1) {
      float eTa = MLX90640_GetTa(frameData, &mlx90640) - 8;
      float emissivity = 0.95;


      MLX90640_CalculateTo(frameData, &mlx90640, emissivity, eTa, frameTemperatureCore0);
      //
      // NOTE: leaving this out because we do have bad pixels and this breaks it
      // MLX90640_BadPixelsCorrection((&mlx90640)->brokenPixels, frameTemperatureCore0, 1, &mlx90640);
      // MLX90640_BadPixelsCorrection((&mlx90640)->outlierPixels, frameTemperatureCore0, 1, &mlx90640);
      // signal to the other thread that we're ready to draw the next frame

      mutex_enter_blocking(&mutex);

      frameTemperatureChanged = true;
      for (int i = 0; i < MLX90640_PIXEL_NUM; i++) {
        frameTemperatureCore1[i] = frameTemperatureCore0[i];
      }

      mutex_exit(&mutex);

    } else {
      printf("MLX90640 failed while getting frame data (%d).\n", subpage);
    }
  }
}
