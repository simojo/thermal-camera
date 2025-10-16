#include <pico/stdio.h>
#include <stdio.h>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "mlx90640/MLX90640_API.h"
#include "scanner.h"

#define EEPROM_DATA_LEN 832
#define MLX90640_ADDR 0x33
#define MLX90640_PIXEL_COUNT 768

const char *heatmap_ascii = " .:-=+*#%@";

void printEEPROM() {
  uint16_t eeprom_data[EEPROM_DATA_LEN];
  if (MLX90640_DumpEE(MLX90640_ADDR, eeprom_data) == 0) {
    for (int i = 0; i < 16; i++) {
      if (i == 0) {
        printf("       ");
      }
      printf("%02X ", i);
    }
    printf("\n");
    for (int i = 0; i < 16; i++) {
      if (i == 0) {
        printf("-----|-");
      }
      printf("---");
    }
    printf("\n");
    for (int i = 0; i < EEPROM_DATA_LEN; i += 1) {
      char ending = ' ';
      if (i % 16 == 0) {
        printf("%04X | ", i);
      }
      if ((i + 1) % 16 == 0) {
        ending = '\n';
      }
      // rp2040 is little-endian, so LSB comes first in sequence
      uint8_t HSB = (eeprom_data[i] >> 0) & 0xFF;
      uint8_t LSB = (eeprom_data[i] >> 8) & 0xFF;
      printf("%02X %02X%c", HSB, LSB, ending);
    }
  } else {
    printf("MLX90640 failed while getting EEPROM data.\n");
  }
}

paramsMLX90640 mlx90640;
float frameTemperature[MLX90640_PIXEL_COUNT];
uint16_t frameData[MLX90640_PIXEL_COUNT + 64 + 2];

int main() {
  stdio_init_all();

  printf("here0");
  MLX90640_SetRefreshRate(MLX90640_ADDR, 0b010);
  printf("here1");
  MLX90640_SetChessMode(MLX90640_ADDR);
  printf("here2");
  // MLX90640_SetSubPageRepeat(MLX90640_ADDR, 0);
  uint16_t eeData[EEPROM_DATA_LEN];
  MLX90640_DumpEE(MLX90640_ADDR, eeData);
  printf("here3");
  MLX90640_ExtractParameters(eeData, &mlx90640);
  printf("here4");

  // get user input
  char c = '\0';
  while (1) {
    printf("Press 'q' to quit: ");
    c = getchar_timeout_us((uint32_t)1e6);
    if (c == '\r' || c == '\n') {
      break;
    } else if (c == 'q' || c == 'Q') {
      while (1);
    }
    printf("\r");
  }
  printf("\n");

  while (1) {
    if (MLX90640_GetFrameData(MLX90640_ADDR, frameData) == 0) {
      float eTa = MLX90640_GetTa(frameData, &mlx90640) - 8;
      float emissivity = 0.98;
      MLX90640_CalculateTo(frameData, &mlx90640, emissivity, eTa, frameTemperature);
      uint16_t max = 0;
      uint16_t min = 0xFFFF;
      // get max value in frame data
      for (int i = 0; i < MLX90640_PIXEL_COUNT; i++) {
        max = frameTemperature[i] > max ? frameTemperature[i] : max;
      }
      // get min value in frame data
      for (int i = 0; i < MLX90640_PIXEL_COUNT; i++) {
        min = frameTemperature[i] < min ? frameTemperature[i] : min;
      }
      float delta = max - min > 0 ? max - min : 1;
      for (int i = 0; i < MLX90640_PIXEL_COUNT; i += 1) {
        uint8_t heatmap_index = (int)((frameTemperature[i] - min) * 10.0f / delta);
        printf("%c%c", heatmap_ascii[heatmap_index], heatmap_ascii[heatmap_index]);
        if ((i + 1) % 32 == 0) {
          printf("\n");
        }
      }
    } else {
      printf("MLX90640 failed while getting frame data.\n");
    }
  }
}
