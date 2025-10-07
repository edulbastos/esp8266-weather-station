#include "ssd1306.h"
#include <string.h>

// Simple 5x7 font
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // backslash
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // f
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // z
};

void ssd1306_draw_char(int16_t x, int16_t y, char c, uint8_t size)
{
    if (c < ' ' || c > 'z') {
        c = ' ';
    }
    
    const uint8_t *glyph = font5x7[c - ' '];
    
    for (uint8_t i = 0; i < 5; i++) {
        uint8_t line = glyph[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (line & (1 << j)) {
                if (size == 1) {
                    ssd1306_draw_pixel(x + i, y + j, true);
                } else {
                    // Draw larger pixel
                    for (uint8_t sx = 0; sx < size; sx++) {
                        for (uint8_t sy = 0; sy < size; sy++) {
                            ssd1306_draw_pixel(x + i * size + sx, y + j * size + sy, true);
                        }
                    }
                }
            }
        }
    }
}

void ssd1306_draw_string(int16_t x, int16_t y, const char *str, uint8_t size)
{
    int16_t cursor_x = x;
    
    while (*str) {
        if (*str == '\n') {
            cursor_x = x;
            y += 8 * size;
        } else {
            ssd1306_draw_char(cursor_x, y, *str, size);
            cursor_x += 6 * size;
        }
        str++;
    }
}

void ssd1306_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool color)
{
    int16_t dx = x1 - x0;
    int16_t dy = y1 - y0;
    int16_t steps = (dx > dy) ? (dx > -dx ? dx : -dx) : (dy > -dy ? dy : -dy);
    
    if (steps == 0) {
        ssd1306_draw_pixel(x0, y0, color);
        return;
    }
    
    float x_inc = (float)dx / steps;
    float y_inc = (float)dy / steps;
    float x = x0;
    float y = y0;
    
    for (int i = 0; i <= steps; i++) {
        ssd1306_draw_pixel((int16_t)x, (int16_t)y, color);
        x += x_inc;
        y += y_inc;
    }
}

void ssd1306_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, bool color)
{
    ssd1306_draw_line(x, y, x + w - 1, y, color);
    ssd1306_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
    ssd1306_draw_line(x + w - 1, y + h - 1, x, y + h - 1, color);
    ssd1306_draw_line(x, y + h - 1, x, y, color);
}

void ssd1306_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, bool color)
{
    for (int16_t i = x; i < x + w; i++) {
        for (int16_t j = y; j < y + h; j++) {
            ssd1306_draw_pixel(i, j, color);
        }
    }
}

// WiFi icon - simple signal bars
void ssd1306_draw_wifi_icon(int16_t x, int16_t y, bool connected)
{
    if (connected) {
        // Draw 4 bars of increasing height (signal strength indicator)
        // Bar 1 (shortest)
        ssd1306_fill_rect(x, y + 9, 2, 3, true);

        // Bar 2
        ssd1306_fill_rect(x + 3, y + 6, 2, 6, true);

        // Bar 3
        ssd1306_fill_rect(x + 6, y + 3, 2, 9, true);

        // Bar 4 (tallest)
        ssd1306_fill_rect(x + 9, y, 2, 12, true);
    }
    // No display when disconnected - absence of icon indicates no WiFi
}

// Weather icons (16x16 pixels)
void ssd1306_draw_weather_icon(int16_t x, int16_t y, int weather_type)
{
    switch(weather_type) {
        case ICON_CLEAR: // Sun
            // Draw circle (sun)
            for (int i = -4; i <= 4; i++) {
                for (int j = -4; j <= 4; j++) {
                    if (i*i + j*j <= 16 && i*i + j*j >= 9) {
                        ssd1306_draw_pixel(x + 8 + i, y + 8 + j, true);
                    }
                }
            }
            // Sun rays
            ssd1306_draw_line(x + 8, y, x + 8, y + 3, true);
            ssd1306_draw_line(x + 8, y + 13, x + 8, y + 16, true);
            ssd1306_draw_line(x, y + 8, x + 3, y + 8, true);
            ssd1306_draw_line(x + 13, y + 8, x + 16, y + 8, true);
            ssd1306_draw_line(x + 2, y + 2, x + 4, y + 4, true);
            ssd1306_draw_line(x + 12, y + 12, x + 14, y + 14, true);
            ssd1306_draw_line(x + 12, y + 4, x + 14, y + 2, true);
            ssd1306_draw_line(x + 2, y + 14, x + 4, y + 12, true);
            break;
            
        case ICON_CLOUDS: // Cloud - improved with better shape
            // Bottom of cloud
            ssd1306_fill_rect(x + 2, y + 10, 12, 2, true);
            // Middle sections
            ssd1306_fill_rect(x + 1, y + 8, 14, 2, true);
            // Top bumps (three circles effect)
            ssd1306_fill_rect(x + 3, y + 6, 4, 2, true);   // Left bump
            ssd1306_fill_rect(x + 6, y + 5, 4, 2, true);   // Center bump (higher)
            ssd1306_fill_rect(x + 9, y + 6, 4, 2, true);   // Right bump
            // Round the top
            ssd1306_draw_pixel(x + 7, y + 4, true);
            ssd1306_draw_pixel(x + 8, y + 4, true);
            break;
            
        case ICON_RAIN: // Rain
            // Cloud
            ssd1306_draw_line(x + 3, y + 6, x + 13, y + 6, true);
            ssd1306_fill_rect(x + 3, y + 4, 11, 2, true);
            // Rain drops
            ssd1306_draw_pixel(x + 4, y + 9, true);
            ssd1306_draw_pixel(x + 4, y + 11, true);
            ssd1306_draw_pixel(x + 8, y + 8, true);
            ssd1306_draw_pixel(x + 8, y + 10, true);
            ssd1306_draw_pixel(x + 8, y + 12, true);
            ssd1306_draw_pixel(x + 12, y + 9, true);
            ssd1306_draw_pixel(x + 12, y + 11, true);
            break;
            
        case ICON_THUNDERSTORM: // Thunderstorm
            // Cloud
            ssd1306_draw_line(x + 3, y + 4, x + 13, y + 4, true);
            ssd1306_fill_rect(x + 3, y + 2, 11, 2, true);
            // Lightning bolt
            ssd1306_draw_line(x + 9, y + 5, x + 7, y + 9, true);
            ssd1306_draw_line(x + 7, y + 9, x + 9, y + 9, true);
            ssd1306_draw_line(x + 9, y + 9, x + 7, y + 14, true);
            break;
            
        case ICON_SNOW: // Snow
            // Cloud
            ssd1306_draw_line(x + 3, y + 5, x + 13, y + 5, true);
            ssd1306_fill_rect(x + 3, y + 3, 11, 2, true);
            // Snowflakes
            ssd1306_draw_pixel(x + 4, y + 8, true);
            ssd1306_draw_pixel(x + 4, y + 12, true);
            ssd1306_draw_pixel(x + 8, y + 10, true);
            ssd1306_draw_pixel(x + 8, y + 14, true);
            ssd1306_draw_pixel(x + 12, y + 8, true);
            ssd1306_draw_pixel(x + 12, y + 12, true);
            break;
            
        case ICON_MIST: // Mist/Fog
            // Horizontal lines
            ssd1306_draw_line(x + 2, y + 4, x + 14, y + 4, true);
            ssd1306_draw_line(x + 1, y + 7, x + 13, y + 7, true);
            ssd1306_draw_line(x + 3, y + 10, x + 15, y + 10, true);
            ssd1306_draw_line(x + 2, y + 13, x + 14, y + 13, true);
            break;
            
        default: // Unknown
            ssd1306_draw_rect(x, y, 16, 16, true);
            ssd1306_draw_string(x + 5, y + 5, "?", 1);
            break;
    }
}

// Draw weather icon at 2x scale (32x32 instead of 16x16)
void ssd1306_draw_weather_icon_large(int16_t x, int16_t y, int weather_type)
{
    switch(weather_type) {
        case ICON_CLEAR: // Sun (2x size)
            // Draw circle (sun) - 2x radius
            for (int i = -8; i <= 8; i++) {
                for (int j = -8; j <= 8; j++) {
                    if (i*i + j*j <= 64 && i*i + j*j >= 36) {
                        ssd1306_draw_pixel(x + 16 + i, y + 16 + j, true);
                    }
                }
            }
            // Sun rays (2x length)
            ssd1306_draw_line(x + 16, y, x + 16, y + 6, true);
            ssd1306_draw_line(x + 16, y + 26, x + 16, y + 32, true);
            ssd1306_draw_line(x, y + 16, x + 6, y + 16, true);
            ssd1306_draw_line(x + 26, y + 16, x + 32, y + 16, true);
            ssd1306_draw_line(x + 4, y + 4, x + 8, y + 8, true);
            ssd1306_draw_line(x + 24, y + 24, x + 28, y + 28, true);
            ssd1306_draw_line(x + 24, y + 8, x + 28, y + 4, true);
            ssd1306_draw_line(x + 4, y + 28, x + 8, y + 24, true);
            break;
            
        case ICON_CLOUDS: // Cloud (2x size) - improved with better shape
            // Bottom of cloud
            ssd1306_fill_rect(x + 4, y + 20, 24, 4, true);
            // Middle sections
            ssd1306_fill_rect(x + 2, y + 16, 28, 4, true);
            // Top bumps (three circles effect)
            ssd1306_fill_rect(x + 6, y + 12, 8, 4, true);   // Left bump
            ssd1306_fill_rect(x + 12, y + 10, 8, 4, true);  // Center bump (higher)
            ssd1306_fill_rect(x + 18, y + 12, 8, 4, true);  // Right bump
            // Round the top
            ssd1306_fill_rect(x + 14, y + 8, 4, 2, true);
            break;
            
        case ICON_RAIN: // Rain (2x size)
            // Cloud
            ssd1306_draw_line(x + 6, y + 12, x + 26, y + 12, true);
            ssd1306_fill_rect(x + 6, y + 8, 22, 4, true);
            // Rain drops (2x spacing)
            ssd1306_draw_pixel(x + 8, y + 18, true);
            ssd1306_draw_pixel(x + 8, y + 22, true);
            ssd1306_draw_pixel(x + 16, y + 16, true);
            ssd1306_draw_pixel(x + 16, y + 20, true);
            ssd1306_draw_pixel(x + 16, y + 24, true);
            ssd1306_draw_pixel(x + 24, y + 18, true);
            ssd1306_draw_pixel(x + 24, y + 22, true);
            break;
            
        case ICON_THUNDERSTORM: // Thunderstorm (2x size)
            // Cloud
            ssd1306_draw_line(x + 6, y + 8, x + 26, y + 8, true);
            ssd1306_fill_rect(x + 6, y + 4, 22, 4, true);
            // Lightning bolt (2x size)
            ssd1306_draw_line(x + 18, y + 10, x + 14, y + 18, true);
            ssd1306_draw_line(x + 14, y + 18, x + 18, y + 18, true);
            ssd1306_draw_line(x + 18, y + 18, x + 14, y + 28, true);
            break;
            
        case ICON_SNOW: // Snow (2x size)
            // Cloud
            ssd1306_draw_line(x + 6, y + 10, x + 26, y + 10, true);
            ssd1306_fill_rect(x + 6, y + 6, 22, 4, true);
            // Snowflakes (2x spacing)
            ssd1306_draw_pixel(x + 8, y + 16, true);
            ssd1306_draw_pixel(x + 8, y + 24, true);
            ssd1306_draw_pixel(x + 16, y + 20, true);
            ssd1306_draw_pixel(x + 16, y + 28, true);
            ssd1306_draw_pixel(x + 24, y + 16, true);
            ssd1306_draw_pixel(x + 24, y + 24, true);
            break;
            
        case ICON_MIST: // Mist/Fog (2x size)
            // Horizontal lines (2x spacing)
            ssd1306_draw_line(x + 4, y + 8, x + 28, y + 8, true);
            ssd1306_draw_line(x + 2, y + 14, x + 26, y + 14, true);
            ssd1306_draw_line(x + 6, y + 20, x + 30, y + 20, true);
            ssd1306_draw_line(x + 4, y + 26, x + 28, y + 26, true);
            break;
            
        default: // Unknown (2x size)
            ssd1306_draw_rect(x, y, 32, 32, true);
            ssd1306_draw_string(x + 10, y + 10, "?", 2);
            break;
    }
}
