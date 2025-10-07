#include "dht22.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"

static const char *TAG = "DHT22";

static float last_temperature = 0.0;
static float last_humidity = 0.0;
static bool data_valid = false;

#define DHT_GPIO CONFIG_DHT22_GPIO

// DHT22 timing (in microseconds)
#define DHT_TIMEOUT_US 2000  
#define DHT_START_SIGNAL_US 1100
#define DHT_MAX_RETRIES 3

static int dht_await_pin_state(uint32_t timeout, int expected_state)
{
    for (uint32_t i = 0; i < timeout; i++) {
        if (gpio_get_level(DHT_GPIO) == expected_state) {
            return i;
        }
        ets_delay_us(1);
    }
    return -1;
}

static esp_err_t dht_read_raw(uint8_t data[5])
{
    memset(data, 0, 5);

    // Disable interrupts during critical timing section
    taskDISABLE_INTERRUPTS();
    
    // Send start signal
    gpio_set_direction(DHT_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_GPIO, 0);
    ets_delay_us(DHT_START_SIGNAL_US);
    gpio_set_level(DHT_GPIO, 1);
    ets_delay_us(30);
    gpio_set_direction(DHT_GPIO, GPIO_MODE_INPUT);

    // Wait for DHT response
    if (dht_await_pin_state(DHT_TIMEOUT_US, 0) == -1) {
        taskENABLE_INTERRUPTS();
        return ESP_ERR_TIMEOUT;
    }
    if (dht_await_pin_state(DHT_TIMEOUT_US, 1) == -1) {
        taskENABLE_INTERRUPTS();
        return ESP_ERR_TIMEOUT;
    }
    if (dht_await_pin_state(DHT_TIMEOUT_US, 0) == -1) {
        taskENABLE_INTERRUPTS();
        return ESP_ERR_TIMEOUT;
    }

    // Read 40 bits of data
    for (int i = 0; i < 40; i++) {
        if (dht_await_pin_state(DHT_TIMEOUT_US, 1) == -1) {
            taskENABLE_INTERRUPTS();
            return ESP_ERR_TIMEOUT;
        }

        int high_time = dht_await_pin_state(DHT_TIMEOUT_US, 0);
        if (high_time == -1) {
            taskENABLE_INTERRUPTS();
            return ESP_ERR_TIMEOUT;
        }

        // If high time > 40us, it's a '1', otherwise it's a '0'
        if (high_time > 40) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    // Re-enable interrupts
    taskENABLE_INTERRUPTS();

    return ESP_OK;
}

static void dht22_task(void *pvParameters)
{
    uint8_t data[5];
    int retry_count = 0;
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(CONFIG_DHT22_READ_INTERVAL * 1000));

        esp_err_t err = ESP_FAIL;
        
        // Try up to DHT_MAX_RETRIES times
        for (retry_count = 0; retry_count < DHT_MAX_RETRIES; retry_count++) {
            if (retry_count > 0) {
                ESP_LOGW(TAG, "Retry %d/%d", retry_count, DHT_MAX_RETRIES);
                vTaskDelay(pdMS_TO_TICKS(500));  // Wait before retry
            }
            
            err = dht_read_raw(data);
            if (err == ESP_OK) {
                break;  // Success, exit retry loop
            }
        }
        
        if (err == ESP_OK) {
            // Verify checksum
            uint8_t checksum = data[0] + data[1] + data[2] + data[3];
            if (checksum == data[4]) {
                // Calculate humidity and temperature
                uint16_t raw_humidity = (data[0] << 8) | data[1];
                uint16_t raw_temperature = (data[2] << 8) | data[3];
                
                last_humidity = raw_humidity / 10.0;
                
                // Check if temperature is negative
                if (raw_temperature & 0x8000) {
                    raw_temperature &= 0x7FFF;
                    last_temperature = -(raw_temperature / 10.0);
                } else {
                    last_temperature = raw_temperature / 10.0;
                }
                
                data_valid = true;
                
                // Log temperature as integer to avoid float printf issues
                int temp_int = (int)last_temperature;
                int hum_int = (int)last_humidity;
                ESP_LOGI(TAG, "Temperature: %dC, Humidity: %d%%", temp_int, hum_int);
            } else {
                ESP_LOGW(TAG, "Checksum error (got 0x%02X, expected 0x%02X)", 
                         data[4], checksum);
                // Don't invalidate data on checksum error, keep last valid reading
            }
        } else {
            if (retry_count >= DHT_MAX_RETRIES) {
                ESP_LOGW(TAG, "Failed to read DHT22 after %d retries", DHT_MAX_RETRIES);
                // Don't invalidate data, keep last valid reading
            }
        }
    }
}

void dht22_init(void)
{
    gpio_set_direction(DHT_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(DHT_GPIO, GPIO_PULLUP_ONLY);
    
    // Wait for sensor to stabilize
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Create task to read sensor periodically
    xTaskCreate(dht22_task, "dht22_task", 2048, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "DHT22 initialized on GPIO%d", DHT_GPIO);
}

esp_err_t dht22_read(float *temperature, float *humidity)
{
    if (!data_valid) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (temperature != NULL) {
        *temperature = last_temperature;
    }
    if (humidity != NULL) {
        *humidity = last_humidity;
    }
    
    return ESP_OK;
}

float dht22_get_temperature(void)
{
    return last_temperature;
}

float dht22_get_humidity(void)
{
    return last_humidity;
}

bool dht22_is_valid(void)
{
    return data_valid;
}
