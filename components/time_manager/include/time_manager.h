#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <time.h>
#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Initialize time manager and sync with NTP
 */
void time_manager_init(void);

/**
 * @brief Get current local time
 * @param timeinfo Pointer to struct tm to store time
 * @return ESP_OK on success
 */
esp_err_t time_manager_get_time(struct tm *timeinfo);

/**
 * @brief Check if time is synchronized
 * @return true if time is synced, false otherwise
 */
bool time_is_synced(void);

#endif // TIME_MANAGER_H
