#include "ssd1306.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "dht22.h"
#include "weather_api.h"
#include "wifi_manager.h"
#include "time_manager.h"

static const char *TAG = "SSD1306";

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000
#define SSD1306_ADDR CONFIG_SSD1306_I2C_ADDR

static uint8_t display_buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

// SSD1306 commands
#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA
#define SSD1306_SETVCOMDETECT 0xDB
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9
#define SSD1306_SETMULTIPLEX 0xA8
#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10
#define SSD1306_SETSTARTLINE 0x40
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR 0x22
#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8
#define SSD1306_SEGREMAP 0xA0
#define SSD1306_CHARGEPUMP 0x8D

static esp_err_t ssd1306_write_command(uint8_t command)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SSD1306_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true);  // Command mode
    i2c_master_write_byte(cmd, command, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t ssd1306_write_data(uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SSD1306_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x40, true);  // Data mode
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static void ssd1306_init_display(void)
{
    // Initialize I2C
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = CONFIG_SSD1306_SDA_GPIO;
    conf.scl_io_num = CONFIG_SSD1306_SCL_GPIO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.clk_stretch_tick = 300; // Maximum wait time for clock stretch
    
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode));
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));

    vTaskDelay(pdMS_TO_TICKS(100));

    // Initialize display
    ssd1306_write_command(SSD1306_DISPLAYOFF);
    ssd1306_write_command(SSD1306_SETDISPLAYCLOCKDIV);
    ssd1306_write_command(0x80);
    ssd1306_write_command(SSD1306_SETMULTIPLEX);
    ssd1306_write_command(SSD1306_HEIGHT - 1);
    ssd1306_write_command(SSD1306_SETDISPLAYOFFSET);
    ssd1306_write_command(0x00);
    ssd1306_write_command(SSD1306_SETSTARTLINE | 0x00);
    ssd1306_write_command(SSD1306_CHARGEPUMP);
    ssd1306_write_command(0x14);
    ssd1306_write_command(SSD1306_MEMORYMODE);
    ssd1306_write_command(0x00);
    ssd1306_write_command(SSD1306_SEGREMAP | 0x01);
    ssd1306_write_command(SSD1306_COMSCANDEC);
    ssd1306_write_command(SSD1306_SETCOMPINS);
    ssd1306_write_command(0x12);
    ssd1306_write_command(SSD1306_SETCONTRAST);
    ssd1306_write_command(0xCF);
    ssd1306_write_command(SSD1306_SETPRECHARGE);
    ssd1306_write_command(0xF1);
    ssd1306_write_command(SSD1306_SETVCOMDETECT);
    ssd1306_write_command(0x40);
    ssd1306_write_command(SSD1306_DISPLAYALLON_RESUME);
    ssd1306_write_command(SSD1306_NORMALDISPLAY);
    ssd1306_write_command(SSD1306_DISPLAYON);

    ESP_LOGI(TAG, "SSD1306 display initialized");
}

void ssd1306_clear(void)
{
    memset(display_buffer, 0, sizeof(display_buffer));
}

void ssd1306_display(void)
{
    ssd1306_write_command(SSD1306_COLUMNADDR);
    ssd1306_write_command(0);
    ssd1306_write_command(SSD1306_WIDTH - 1);
    ssd1306_write_command(SSD1306_PAGEADDR);
    ssd1306_write_command(0);
    ssd1306_write_command((SSD1306_HEIGHT / 8) - 1);

    for (uint16_t i = 0; i < (SSD1306_WIDTH * SSD1306_HEIGHT / 8); i += 16) {
        ssd1306_write_data(&display_buffer[i], 16);
    }
}

void ssd1306_draw_pixel(int16_t x, int16_t y, bool color)
{
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) {
        return;
    }

    if (color) {
        display_buffer[x + (y / 8) * SSD1306_WIDTH] |= (1 << (y & 7));
    } else {
        display_buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y & 7));
    }
}

static void draw_weather_screen(void)
{
    ssd1306_clear();
    
    // Get current time
    struct tm timeinfo;
    char time_str[16];
    bool time_valid = false;
    if (time_manager_get_time(&timeinfo) == ESP_OK) {
        snprintf(time_str, sizeof(time_str), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        time_valid = true;
    } else {
        snprintf(time_str, sizeof(time_str), "--:--");
    }
    
    // ===== FIRST LINE (y=2): TIME | DHT22 TEMP | WIFI =====
    // Draw time on the left
    ssd1306_draw_string(2, 2, time_str, 1);
    
    // Draw DHT22 temperature in the center
    char temp_str[16];
    if (dht22_is_valid()) {
        float dht_temp = dht22_get_temperature();
        int temp_whole = (int)dht_temp;
        int temp_decimal = (int)((dht_temp - temp_whole) * 10 + 0.5);
        if (temp_decimal >= 10) {
            temp_whole += 1;
            temp_decimal = 0;
        }
        snprintf(temp_str, sizeof(temp_str), "%d.%dC", temp_whole, temp_decimal);
    } else {
        snprintf(temp_str, sizeof(temp_str), "--.-C");
    }
    // Centralize DHT22 temp
    int text_width = strlen(temp_str) * 6;
    int x_centered = (128 - text_width) / 2;
    ssd1306_draw_string(x_centered, 2, temp_str, 1);
    
    // Draw WiFi icon on the right
    ssd1306_draw_wifi_icon(110, 2, wifi_is_connected());
    
    // Get weather forecast
    weather_forecast_t forecast[3];
    bool has_weather = weather_is_valid();
    
    // Array of weekday names
    const char* days_short[] = {"Dom", "Seg", "Ter", "Qua", "Qui", "Sex", "Sab"};
    
    if (has_weather) {
        weather_get_forecast(0, &forecast[0]);  // Today
        weather_get_forecast(1, &forecast[1]);  // Tomorrow
        weather_get_forecast(2, &forecast[2]);  // Day after tomorrow
    }
    
    // ===== TODAY (LARGER/HIGHLIGHTED) - LEFT =====
    if (has_weather) {
        // Draw current weather icon LARGE (2x size: 32x32) - moved up 5 pixels
        ssd1306_draw_weather_icon_large(4, 9, forecast[0].condition);

        // Draw current temperature (large font, below icon) - subido 8 pixels
        char temp[16];
        int temp_whole = (int)forecast[0].temp;
        int temp_decimal = (int)((forecast[0].temp - temp_whole) * 10);
        if (temp_decimal < 0) temp_decimal = -temp_decimal;

        if (temp_whole != 0 || forecast[0].temp > 0.5) {
            snprintf(temp, sizeof(temp), "%dC", temp_whole);
        } else {
            snprintf(temp, sizeof(temp), "0C");
        }
        ssd1306_draw_string(4, 38, temp, 2);  // Large font - moved up 8 pixels (46 - 4 - 3 - 1 = 38)

        // Draw day of week at bottom of screen (small font)
        if (time_valid) {
            int current_day = timeinfo.tm_wday;
            // Centralize day name under temperature
            int day_width = 3 * 6;  // 3 chars * 6 pixels
            int temp_width = strlen(temp) * 12;  // Large font is ~12 pixels per char
            int x_day = 4 + (temp_width - day_width) / 2;
            ssd1306_draw_string(x_day, 56, days_short[current_day], 1);  // y=56 (altura da fonte é 8, então vai até y=63)
        }
    } else {
        ssd1306_draw_string(8, 27, "N/A", 2);
    }
    
    // ===== NEXT 2 PERIODS (SMALLER) - RIGHT =====
    int x_start = 64;  // Start on right half of screen
    int y_icon = 19;   // Icon position (moved down 5 pixels: 14 + 5 = 19)
    int y_temp = 41;   // Temperature position (moved down 5 pixels: 36 + 5 = 41)
    int y_day = 53;    // Day of week position (moved down 7 pixels: 46 + 5 + 2 = 53)
    int spacing = 32;  // Space between forecast periods

    for (int i = 1; i <= 2; i++) {  // Only show 2 future periods (indexes 1 and 2)
        int x = x_start + (i - 1) * spacing;

        if (has_weather) {
            // Draw small weather icon (16x16)
            ssd1306_draw_weather_icon(x, y_icon, forecast[i].condition);

            // Draw temperature below icon (small font)
            char temp[16];
            int temp_whole = (int)forecast[i].temp;

            if (temp_whole != 0 || forecast[i].temp > 0.5) {
                snprintf(temp, sizeof(temp), "%dC", temp_whole);
            } else {
                snprintf(temp, sizeof(temp), "0C");
            }

            // Centralize temperature under icon (icon is 16px wide)
            int temp_width = strlen(temp) * 6;  // Font size 1 is ~6 pixels per char
            int x_temp = x + (16 - temp_width) / 2;
            ssd1306_draw_string(x_temp, y_temp, temp, 1);

            // Draw day of week below temperature (centralized)
            if (time_valid) {
                // Calculate future day (tomorrow and day after tomorrow)
                int future_day = (timeinfo.tm_wday + i) % 7;
                int day_width = 3 * 6;  // 3 chars * 6 pixels
                int x_day_pos = x + (16 - day_width) / 2;
                ssd1306_draw_string(x_day_pos, y_day, days_short[future_day], 1);
            }
        } else {
            ssd1306_draw_string(x, y_icon + 6, "N/A", 1);
        }
    }
    
    ssd1306_display();
}

static void display_update_task(void *pvParameters)
{
    while (1) {
        draw_weather_screen();
        vTaskDelay(pdMS_TO_TICKS(CONFIG_DISPLAY_UPDATE_INTERVAL * 1000));
    }
}

void ssd1306_init(void)
{
    ssd1306_init_display();
    
    // Ensure display is completely cleared after reboot
    // Clear buffer and send to display multiple times to ensure it's blank
    ssd1306_clear();
    ssd1306_display();
    vTaskDelay(pdMS_TO_TICKS(50));
    
    ssd1306_clear();
    ssd1306_display();
    vTaskDelay(pdMS_TO_TICKS(50));
    
    ESP_LOGI(TAG, "Display cleared after reboot");
    
    // Create task to update display periodically
    xTaskCreate(display_update_task, "display_update", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "SSD1306 task started");
}
