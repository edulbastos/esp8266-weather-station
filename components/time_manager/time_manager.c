#include "time_manager.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "lwip/apps/sntp.h"

static const char *TAG = "TIME_MANAGER";
static bool time_synced = false;

static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Time synchronized with NTP server");
    time_synced = true;
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, CONFIG_NTP_SERVER);
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_init();
}

static void wait_for_time_sync(void)
{
    int retry = 0;
    const int retry_count = 30;
    
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    
    if (retry >= retry_count) {
        ESP_LOGW(TAG, "Failed to sync time with NTP server");
    }
}

void time_manager_init(void)
{
    initialize_sntp();
    wait_for_time_sync();
    
    // Set timezone
    setenv("TZ", CONFIG_TIMEZONE, 1);
    tzset();
    
    // Create task to periodically resync time
    // This is handled by SNTP automatically, but we log it
    time_synced = (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED);
}

esp_err_t time_manager_get_time(struct tm *timeinfo)
{
    if (timeinfo == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    time_t now;
    time(&now);
    localtime_r(&now, timeinfo);
    
    // Check if time is valid (year > 2020)
    if (timeinfo->tm_year < (2020 - 1900)) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return ESP_OK;
}

bool time_is_synced(void)
{
    struct tm timeinfo;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Simple check: if year is reasonable, time is synced
    return (timeinfo.tm_year >= (2020 - 1900));
}
