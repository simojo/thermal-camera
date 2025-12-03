/*
 * main.c
 *
 * @copyright Copyright (C) 2025 Simon J. Jones <github@simonjjones.com>
 * Licensed under the Apache License, Version 2.0.
 */

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
#include "pico/cyw43_arch.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include "wifi_scan.h"

#define MLX90640_ADDR 0x33
#define INITIAL_DELAY_MS 6000

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

  // display a loading animation while core0 starts up.
  uint32_t t0_ms = time_us_32() / 1000;
  uint32_t t1_ms = t0_ms;
  uint32_t loading_ani_t0_ms = t0_ms;
  const unsigned int loading_ani_tick_period_ms = 10;
  while (t1_ms - t0_ms < INITIAL_DELAY_MS) {
    t1_ms = time_us_32() / 1000;
    // each 200ms, tick the loading animation
    if (t1_ms - loading_ani_t0_ms > loading_ani_tick_period_ms) {
      st7789_loading_ani_tick();
      loading_ani_t0_ms = time_us_32() / 1000;
    }
  }
  // clear any remaining loading animations
  st7789_framebuf_fill_rect(0, 0, ST7789_LINE_SIZE-1, ST7789_COLUMN_SIZE-1, BLACK);

  while (1) {
    mutex_enter_blocking(&mutex);
    if (frameTemperatureChanged) {
      st7789_fill_32_24(frameTemperatureCore1);
    }
    // critical section
    mutex_exit(&mutex);
  }
}

    printf("Press 'q' to quit\n");
    cyw43_arch_enable_sta_mode();

    // Start a scan immediately
    bool scan_started = false;
    async_at_time_worker_t scan_worker = { .do_work = scan_worker_fn, .user_data = &scan_started };
    hard_assert(async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &scan_worker, 0));

    bool exit = false;
    while(!exit) {
        int key = getchar_timeout_us(0);
        if (key == 'q' || key == 'Q') {
            exit = true;
        }
        if (!cyw43_wifi_scan_active(&cyw43_state) && scan_started) {
            // Start a scan in 10s
            scan_started = false;
            hard_assert(async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &scan_worker, 10000));
        }
        // the following #ifdef is only here so this same example can be used in multiple modes;
        // you do not need it in your code
#if PICO_CYW43_ARCH_POLL
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer) to check for Wi-Fi driver or lwIP work that needs to be done.
        cyw43_arch_poll();
        // you can poll as often as you like, however if you have nothing else to do you can
        // choose to sleep until either a specified time, or cyw43_arch_poll() has work to do:
        cyw43_arch_wait_for_work_until(at_the_end_of_time);
#else
        // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of some (blocking)
        // work you might be doing.
        sleep_ms(1000);
#endif
    }

    cyw43_arch_deinit();

int main() {
  stdio_init_all();

  // NOTE: WIFI
  if (cyw43_arch_init()) {
      printf("failed to initialise wifi\n");
      return 1;
  }
  cyw43_arch_enable_sta_mode();

  // NOTE: WIFI
  // start a scan immediately
  bool scan_started = false;
  async_at_time_worker_t scan_worker = { .do_work = scan_worker_fn, .user_data = &scan_started };
  hard_assert(async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &scan_worker, 0));

  // initialize the mutex and launch core for handling st7789
  mutex_init(&mutex);
  multicore_launch_core1(core1_main);

  // add delay for the camera to start up
  sleep_ms(INITIAL_DELAY_MS);

  printf("[INFO] set refresh rate.\n");
  MLX90640_SetChessMode(MLX90640_ADDR);
  printf("[INFO] set chess mode.\n");
  MLX90640_SetSubPageRepeat(MLX90640_ADDR, 0);
  printf("[INFO] set subpage repeat.\n");

  if (MLX90640_DumpEE(MLX90640_ADDR, eeData) != 0) {
    printf("[ERROR] DumpEE returned error.\n");
  }
  printf("[INFO] dumpEE.\n");
  // set the refresh rate to be 4Hz
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
    // NOTE: WIFI
    if (!cyw43_wifi_scan_active(&cyw43_state) && scan_started) {
        // Start a scan in 10s
        scan_started = false;
        hard_assert(async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &scan_worker, 10000));
    }

    int subpage = 0;
    subpage = MLX90640_GetFrameData(MLX90640_ADDR, frameData);
    // 0, 1 are valid subpge return values. anything else implies error
    if (subpage == 0 || subpage == 1) {
      float eTa = MLX90640_GetTa(frameData, &mlx90640) - 8;
      float emissivity = 0.95;


      MLX90640_CalculateTo(frameData, &mlx90640, emissivity, eTa, frameTemperatureCore0);

      // NOTE: leaving this out because we do have bad pixels and this breaks it
      // MLX90640_BadPixelsCorrection((&mlx90640)->brokenPixels, frameTemperatureCore0, 1, &mlx90640);
      // MLX90640_BadPixelsCorrection((&mlx90640)->outlierPixels, frameTemperatureCore0, 1, &mlx90640);

      // signal to the core1 that we're ready to draw the next frame
      mutex_enter_blocking(&mutex);

      frameTemperatureChanged = true;
      for (int i = 0; i < MLX90640_PIXEL_NUM; i++) {
        frameTemperatureCore1[i] = frameTemperatureCore0[i];
      }

      mutex_exit(&mutex);

    } else {
      printf("[ERROR] MLX90640 failed while getting frame data (%d).\n", subpage);
    }
  }
}
