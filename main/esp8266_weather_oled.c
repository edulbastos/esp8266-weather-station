#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "dht22.h"
#include "ssd1306.h"
#include "weather_api.h"
#include "wifi_manager.h"
#include "time_manager.h"

static const char *TAG = "WEATHER_STATION";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Weather Station...");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize SSD1306 Display FIRST to clear screen immediately after reboot
    ESP_LOGI(TAG, "Initializing Display...");
    ssd1306_init();

    // Initialize WiFi
    ESP_LOGI(TAG, "Initializing WiFi...");
    wifi_manager_init();

    // Initialize Time
    ESP_LOGI(TAG, "Initializing Time...");
    time_manager_init();

    // Initialize DHT22 Sensor
    ESP_LOGI(TAG, "Initializing DHT22...");
    dht22_init();

    // Initialize Weather API
    ESP_LOGI(TAG, "Initializing Weather API...");
    weather_api_init();

    ESP_LOGI(TAG, "Weather Station initialized successfully!");

    // Main loop handled by individual task managers
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
