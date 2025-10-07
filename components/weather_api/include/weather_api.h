#ifndef WEATHER_API_H
#define WEATHER_API_H

#include <stdbool.h>
#include "esp_err.h"

typedef enum {
    WEATHER_CLEAR = 0,
    WEATHER_CLOUDS,
    WEATHER_RAIN,
    WEATHER_DRIZZLE,
    WEATHER_THUNDERSTORM,
    WEATHER_SNOW,
    WEATHER_MIST,
    WEATHER_UNKNOWN
} weather_condition_t;

typedef struct {
    float temp;
    weather_condition_t condition;
    char description[32];
    int dt;  // Unix timestamp
} weather_forecast_t;

/**
 * @brief Initialize weather API manager
 */
void weather_api_init(void);

/**
 * @brief Get current weather
 * @param forecast Pointer to store weather data
 * @return ESP_OK on success
 */
esp_err_t weather_get_current(weather_forecast_t *forecast);

/**
 * @brief Get forecast for specific day (0=today, 1=tomorrow, 2=day after)
 * @param day Day offset (0, 1, or 2)
 * @param forecast Pointer to store weather data
 * @return ESP_OK on success
 */
esp_err_t weather_get_forecast(int day, weather_forecast_t *forecast);

/**
 * @brief Check if weather data is valid
 * @return true if valid, false otherwise
 */
bool weather_is_valid(void);

#endif // WEATHER_API_H
