#include "stm32f1xx.h"
#include "ssd1306.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

I2C_HandleTypeDef hi2c1;

void SysTick_Handler (void) {
    HAL_IncTick();
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9; // 8 МГц * 9 = 72 МГц
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

static void MX_I2C1_Init(void) {
    // 1. Включаем тактирование порта B и модуля I2C1
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    // 2. Настраиваем ножки PB6 (SCL) и PB7 (SDA)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD; // Важно: Alternate Function Open-Drain!
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // 3. Настраиваем сам модуль I2C1
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 400000; // 400 кГц (быстрый режим Fast Mode для плавного фреймрейта)
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

static void MX_GPIO_Init(void) {
    // 1. Включаем тактирование (подаём питание на порт A)
    __HAL_RCC_GPIOA_CLK_ENABLE();
    // 2. Настраиваем ножку PA0
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT; // Режим цифрового входа (слушаем сигнал)
    GPIO_InitStruct.Pull = GPIO_PULLUP;     // Включаем встроенный подтягивающий резистор к +3.3V
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_GAME_OVER
} game_state_t;

// engine
uint32_t last_frame_time = 0;
const float GRAVITY = 0.25f;
game_state_t state = STATE_MENU;
int16_t score = 0;

// pipe
#define PIPE_WIDTH 12
#define PIPE_GAP 27

int16_t pipe_x = 128;
uint8_t pipe_passed = 0;
int16_t gap_y = 15;

// bird sprite
const uint8_t bird_sprite[8] = {
    0x1C, 0x22, 0xEB, 0xBF, 0xFE, 0x42, 0x3C, 0x00
};
float bird_y = 10.0f;
float bird_vy = 0.0f;

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_I2C1_Init();
    MX_GPIO_Init();
    
    ssd1306_init(&hi2c1);
    ssd1306_update();

    while (1) {
        uint32_t now = HAL_GetTick();
        
        if (now - last_frame_time >= 33) {
            last_frame_time = now;

            // input logic
            static uint8_t btn_prev = GPIO_PIN_SET;
            uint8_t btn_curr = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
            uint8_t btn_clicked = (btn_curr == GPIO_PIN_RESET && btn_prev == GPIO_PIN_SET);
            btn_prev = btn_curr;
            
            // game state logic
            if (state == STATE_GAME_OVER) {
                if (btn_clicked) {
                    state = STATE_PLAYING;
                    score = 0;
                    bird_y = 20.0f;
                    bird_vy = 0.0f;
                    pipe_x = 128;
                    gap_y = 15;
                }
            } else if (state == STATE_PLAYING) {
                if (btn_clicked) {
                    bird_vy = -3.0f;
                }

                bird_vy += GRAVITY;
                bird_y += bird_vy;
                if (bird_y < 0) { bird_y = 0; bird_vy = 0; }
                
                pipe_x -= 1;
                if (pipe_x < -PIPE_WIDTH) {
                    pipe_x = 128;
                    gap_y = 10 + (rand() % 20);
                    pipe_passed = 0;
                }
                
                if ((20 + 8 > pipe_x && 20 < pipe_x + PIPE_WIDTH) && (bird_y < gap_y || (bird_y + 8) > (gap_y + PIPE_GAP))) {
                    state = STATE_GAME_OVER;
                } else if (!pipe_passed && (pipe_x + PIPE_WIDTH < 20)) {
                    pipe_passed = 1;
                    score++;
                }
                
                if (bird_y >= 52) {
                    bird_y = 52;
                    state = STATE_GAME_OVER;
                }
            } else if (state == STATE_MENU) {
                if (btn_clicked) {
                    state = STATE_PLAYING;
                    score = 0;
                }
            }

            
            // game render
            ssd1306_clear();

            if (state == STATE_MENU) {
                ssd1306_draw_string(32, 17, "FLAPPY BIRD", COLOR_WHITE);
                ssd1306_draw_string(23, 35, "PRESS TO JUMP", COLOR_WHITE);
            } else if (state == STATE_PLAYING || state == STATE_GAME_OVER) {
                ssd1306_fill_rect(0, 60, 128, 4, COLOR_WHITE);
                ssd1306_draw_bitmap(20, (int16_t)bird_y, bird_sprite, 8, 8, COLOR_WHITE);
    
                ssd1306_fill_rect(pipe_x, 0, PIPE_WIDTH, gap_y, COLOR_WHITE);
                ssd1306_fill_rect(pipe_x, PIPE_GAP + gap_y, PIPE_WIDTH, 128, COLOR_WHITE);

                char dscore[14] = {};
                snprintf(dscore, sizeof(dscore), "SCORE: %d", score);

                ssd1306_draw_string(2,3, dscore, COLOR_WHITE);
            } 
            if (state == STATE_GAME_OVER) {
                ssd1306_fill_rect(18, 12, 92, 36, COLOR_BLACK);
                ssd1306_draw_rect(18, 12, 92, 36, COLOR_WHITE);

                ssd1306_draw_string(32, 17, "GAME OVER", COLOR_WHITE);
                ssd1306_draw_string(23, 35, "PRESS TO JUMP", COLOR_WHITE);
            }
            

            ssd1306_update();
        }     
    }
}