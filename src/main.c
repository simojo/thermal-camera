#include <pico/stdio.h>
#include <math.h>
#include <stdio.h>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "mlx90640/MLX90640_API.h"
#include "mlx90640/MLX90640_I2C_Driver.h"
#include "scanner.h"

#define MLX90640_ADDR 0x33
#define MLX90640_PIXEL_COUNT 768

const char *heatmap_ascii = " .:-=+*#%@";
const char *heatmap_color[] = {
    "\x1b[38;2;0;0;128m",     // dark blue
    "\x1b[38;2;0;0;255m",     // blue
    "\x1b[38;2;0;128;255m",   // cyan
    "\x1b[38;2;0;255;128m",   // turquoise
    "\x1b[38;2;0;255;0m",     // green
    "\x1b[38;2;128;255;0m",   // yellow-green
    "\x1b[38;2;255;255;0m",   // yellow
    "\x1b[38;2;255;128;0m",   // orange
    "\x1b[38;2;255;64;0m",    // red-orange
    "\x1b[38;2;255;255;255m"  // white (hottest)
};
#define ANSI_RESET "\x1b[0m"

static uint16_t eeData[MLX90640_EEPROM_DUMP_NUM];

void printEEPROM() {
  if (MLX90640_DumpEE(MLX90640_ADDR, eeData) == 0) {
    for (int i = 0; i < 16; i++) {
      if (i == 0) {
        printf("       ");
      }
      printf("%02X    ", i);
    }
    printf("\n");
    for (int i = 0; i < 16; i++) {
      if (i == 0) {
        printf("-----|-");
      }
      printf("------");
    }
    printf("\n");
    for (int i = 0; i < MLX90640_EEPROM_DUMP_NUM; i += 1) {
      char ending = ' ';
      if (i % 16 == 0) {
        printf("%04X | ", i + 0x2400);
      }
      if ((i + 1) % 16 == 0) {
        ending = '\n';
      }
      // rp2040 is little-endian, so LSB comes first in sequence
      uint8_t HSB = (eeData[i] >> 0) & 0xFF;
      uint8_t LSB = (eeData[i] >> 8) & 0xFF;
      printf("%02X %02X%c", HSB, LSB, ending);
    }
  } else {
    printf("MLX90640 failed while getting EEPROM data.\n");
  }
}

paramsMLX90640 mlx90640;
static float frameTemperature[MLX90640_PIXEL_COUNT];
static uint16_t frameData[MLX90640_PIXEL_COUNT + 64 + 2];

int main() {
  stdio_init_all();

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

  printEEPROM();
  printf("Press any character to continue...\n");
  getchar();
  MLX90640_SetRefreshRate(MLX90640_ADDR, 0b001);
  printf("[INFO] set refresh rate.\n");
  MLX90640_SetChessMode(MLX90640_ADDR);
  printf("[INFO] set chess mode.\n");
  MLX90640_SetSubPageRepeat(MLX90640_ADDR, 0);
  printf("[INFO] set subpage repeat.\n");
  if (MLX90640_DumpEE(MLX90640_ADDR, eeData) != 0) {
    printf("[ERROR] DumpEE returned error.\n");
    getchar();
  }
  printf("[INFO] dumpEE.\n");
  if (MLX90640_ExtractParameters(eeData, &mlx90640) != 0) {
    printf("[ERROR] ExtractParameters returned error.\n");
    getchar();
  }
  printf("[INFO] extractparameters.\n");

  // get user input
  char c = '\0';
  while (1) {
    printf("Press 'q' to quit: ");
    c = getchar();
    if (c == '\r' || c == '\n') {
      break;
    } else if (c == 'q' || c == 'Q') {
      while (1);
    }
    printf("\r");
  }
  printf("\n");


  while (1) {
    int subpage = 0;
    subpage = MLX90640_GetFrameData(MLX90640_ADDR, frameData);
    // 0, 1 are valid subpge return values. anything else implies error
    if (subpage == 0 || subpage == 1) {
      float eTa = MLX90640_GetTa(frameData, &mlx90640) - 8;
      float emissivity = 0.95;
      MLX90640_CalculateTo(frameData, &mlx90640, emissivity, eTa, frameTemperature);

      MLX90640_BadPixelsCorrection((&mlx90640)->brokenPixels, frameTemperature, 1, &mlx90640);
      MLX90640_BadPixelsCorrection((&mlx90640)->outlierPixels, frameTemperature, 1, &mlx90640);

      float max = -INFINITY;
      float min = INFINITY;
      // get max, min value in frame data
      for (int i = 0; i < MLX90640_PIXEL_COUNT; i++) {
        if (frameTemperature[i] > max) max = frameTemperature[i];
        if (frameTemperature[i] < min) min = frameTemperature[i];
      }
      float delta = max - min > 0 ? max - min : 1;
      for (int i = 0; i < MLX90640_PIXEL_COUNT; i += 1) {
        uint8_t heatmap_index = (uint8_t)((frameTemperature[i] - min)/delta * 10.0f);
        if (heatmap_index < 0) heatmap_index = 0;
        if (heatmap_index > 9) heatmap_index = 9;
        printf("%s%s%s", heatmap_color[heatmap_index], "██", ANSI_RESET);
        if ((i + 1) % 32 == 0) {
          printf("\n");
        }
      }
    } else {
#ifdef DEBUG
      printf("MLX90640 failed while getting frame data.\n");
#endif
    }
  }
}
