#include "ssd1306.h"
#include "font5x7.h"
#include "string.h"

static uint8_t ssd1306_buffer[SSD1306_BUFFER_SIZE];
static I2C_HandleTypeDef *ssd1306_i2c;

// Очистка экрана в черный
void ssd1306_clear (void) {
    memset(ssd1306_buffer, 0x00, SSD1306_BUFFER_SIZE);
}

// Очистка экрана в белый
void ssd1306_fill (void) {
    memset(ssd1306_buffer, 0xFF, SSD1306_BUFFER_SIZE);
}

// Красим определенный пиксель
void ssd1306_draw_pixel (int16_t x, int16_t y, ssd1306_color_t color) {
    if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT) return;

    uint16_t index = x + ((y / 8) * DISPLAY_WIDTH);
    uint8_t bit = y % 8;

    if (color == COLOR_WHITE) {
        ssd1306_buffer[index] |= (1 << bit);
    } else {
        ssd1306_buffer[index] &= ~(1 << bit);
    }
}

// Отправка команды SSD1306 по шине I2C
void ssd1306_write_command (uint8_t cmd) {
    uint8_t payload[2] = {0x00, cmd};
    HAL_I2C_Master_Transmit(ssd1306_i2c, SSD1306_I2C_ADDR, payload, 2, 10);
}

// Обновляем картинку
void ssd1306_update (void) {
    ssd1306_write_command(0x21);
    ssd1306_write_command(0);
    ssd1306_write_command(127);
    ssd1306_write_command(0x22);
    ssd1306_write_command(0);
    ssd1306_write_command(7);

    HAL_I2C_Mem_Write(
        ssd1306_i2c,
        SSD1306_I2C_ADDR,
        0x40,
        I2C_MEMADD_SIZE_8BIT,
        ssd1306_buffer,
        SSD1306_BUFFER_SIZE,
        100
    );
}

// Инициализируем дисплей для отрисовки
void ssd1306_init (I2C_HandleTypeDef *hi2c) {
    ssd1306_i2c = hi2c;

    HAL_Delay(100);

    ssd1306_write_command(0xAE);
    ssd1306_write_command(0x20);
    ssd1306_write_command(0x00);
    ssd1306_write_command(0xA1);
    ssd1306_write_command(0xC8);
    ssd1306_write_command(0x8D);
    ssd1306_write_command(0x14);
    ssd1306_write_command(0xAF);

    ssd1306_clear();
    ssd1306_update();
}

// Рисуем заполненый квадрат/прямоугольник
void ssd1306_fill_rect (int16_t x, int16_t y, int16_t w, int16_t h, ssd1306_color_t color) {
    for (int16_t px = x; px < x + w; px++) {
        for (int16_t py = y; py < y + h; py++) {
            ssd1306_draw_pixel(px, py, color);
        }
    }
}


// Рисуем пустой квадрат/прямоугольник
void ssd1306_draw_rect (int16_t x, int16_t y, int16_t w, int16_t h, ssd1306_color_t color) {
    for (int16_t px = x; px < x + w; px++) {
        ssd1306_draw_pixel(px, y, color);
        ssd1306_draw_pixel(px, y + h - 1, color);
    }
    for (int16_t py = y; py < y + h; py++) {
        ssd1306_draw_pixel(x, py, color);
        ssd1306_draw_pixel(x + w - 1, py, color);
    }
}

// Рисуем фигуру по карте(спрайт)
void ssd1306_draw_bitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, ssd1306_color_t color) {
    for (int16_t py = 0; py < h; py++) {
        uint8_t line = bitmap[py];
        for (int16_t px = 0; px < w; px++) {
            if (line & (0x80 >> px)) {
                ssd1306_draw_pixel(x + px, y + py, color);
            }
        }
    }
}

// Рисуем символ
void ssd1306_draw_char (int16_t x, int16_t y, char c, ssd1306_color_t color) {
    if (c < 32 || c > 126) c = '?';
    uint8_t index = c - 32;

    for (int16_t col = 0; col < 5; col++) {
        uint8_t line = font5x7[index][col];

        for (int16_t row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                ssd1306_draw_pixel(x + col, y + row, color);
            }
        }
    }
}

// Рисуем текст
void ssd1306_draw_string (int16_t x, int16_t y, const char *str, ssd1306_color_t color) {
    while (*str) {
        ssd1306_draw_char(x, y, *str, color);
        x += 6;
        str++;
    }
}