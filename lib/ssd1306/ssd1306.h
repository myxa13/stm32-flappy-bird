#include "stm32f1xx_hal.h"
#include <stdint.h>

#ifndef SSD1306
#define SSD1306

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

#define SSD1306_BUFFER_SIZE ((DISPLAY_WIDTH * DISPLAY_HEIGHT) / 8)
#define SSD1306_ADDR 0x3C
#define SSD1306_I2C_ADDR (0x3c << 1)

typedef enum {
    COLOR_BLACK = 0,
    COLOR_WHITE = 1
} ssd1306_color_t;


void ssd1306_init (I2C_HandleTypeDef *hi2c);
void ssd1306_update (void);
void ssd1306_fill (void);
void ssd1306_draw_pixel (int16_t x, int16_t y, ssd1306_color_t color);
void ssd1306_clear (void);

void ssd1306_fill_rect (int16_t x, int16_t y, int16_t w, int16_t h, ssd1306_color_t color);
void ssd1306_draw_rect (int16_t x, int16_t y, int16_t w, int16_t h, ssd1306_color_t color);
void ssd1306_draw_bitmap (int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, ssd1306_color_t color);
void ssd1306_draw_char (int16_t x, int16_t y, char c, ssd1306_color_t color);
void ssd1306_draw_string (int16_t x, int16_t y, const char *str, ssd1306_color_t color);

#endif