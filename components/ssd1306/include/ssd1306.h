#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#define SSD1306_WIDTH  128
#define SSD1306_HEIGHT 64

/**
 * @brief Initialize SSD1306 display
 */
void ssd1306_init(void);

/**
 * @brief Clear display buffer
 */
void ssd1306_clear(void);

/**
 * @brief Update display with buffer content
 */
void ssd1306_display(void);

/**
 * @brief Set pixel at position
 */
void ssd1306_draw_pixel(int16_t x, int16_t y, bool color);

/**
 * @brief Draw string at position
 */
void ssd1306_draw_string(int16_t x, int16_t y, const char *str, uint8_t size);

/**
 * @brief Draw weather icon (normal size)
 */
void ssd1306_draw_weather_icon(int16_t x, int16_t y, int weather_type);

/**
 * @brief Draw weather icon (2x larger)
 */
void ssd1306_draw_weather_icon_large(int16_t x, int16_t y, int weather_type);

/**
 * @brief Draw WiFi icon
 */
void ssd1306_draw_wifi_icon(int16_t x, int16_t y, bool connected);

/**
 * @brief Draw line
 */
void ssd1306_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool color);

/**
 * @brief Draw rectangle
 */
void ssd1306_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, bool color);

/**
 * @brief Fill rectangle
 */
void ssd1306_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, bool color);

// Weather icon types
#define ICON_CLEAR       0
#define ICON_CLOUDS      1
#define ICON_RAIN        2
#define ICON_THUNDERSTORM 3
#define ICON_SNOW        4
#define ICON_MIST        5

#endif // SSD1306_H
