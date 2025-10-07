#ifndef DHT22_H
#define DHT22_H

#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Initialize DHT22 sensor
 */
void dht22_init(void);

/**
 * @brief Read temperature and humidity from DHT22
 * @param temperature Pointer to store temperature in Celsius
 * @param humidity Pointer to store relative humidity in percentage
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t dht22_read(float *temperature, float *humidity);

/**
 * @brief Get last read temperature
 * @return Temperature in Celsius
 */
float dht22_get_temperature(void);

/**
 * @brief Get last read humidity
 * @return Relative humidity in percentage
 */
float dht22_get_humidity(void);

/**
 * @brief Check if DHT22 data is valid
 * @return true if valid, false otherwise
 */
bool dht22_is_valid(void);

#endif // DHT22_H
