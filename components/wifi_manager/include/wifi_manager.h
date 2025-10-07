#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Initialize WiFi manager
 */
void wifi_manager_init(void);

/**
 * @brief Check if WiFi is connected
 * @return true if connected, false otherwise
 */
bool wifi_is_connected(void);

/**
 * @brief Get WiFi connection status event group
 */
void* wifi_get_event_group(void);

#endif // WIFI_MANAGER_H
