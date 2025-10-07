#include "weather_api.h"
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"

static const char *TAG = "WEATHER_API";

#define MAX_HTTP_RECV_BUFFER 12288  // 12KB - Increased to handle large forecast responses

static weather_forecast_t current_weather;
static weather_forecast_t forecast_data[3];  // Today, tomorrow, day after
static bool weather_data_valid = false;

static char http_response_buffer[MAX_HTTP_RECV_BUFFER];
static int http_response_len = 0;

// Simple URL encoder for city names (handles spaces and basic special chars)
static void url_encode(const char *src, char *dst, size_t dst_size)
{
    const char *hex = "0123456789ABCDEF";
    size_t j = 0;
    
    for (size_t i = 0; src[i] != '\0' && j < dst_size - 1; i++) {
        unsigned char c = (unsigned char)src[i];
        
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            // Safe characters - no encoding needed
            dst[j++] = c;
        } else if (c == ' ') {
            // Space becomes %20
            if (j + 3 < dst_size) {
                dst[j++] = '%';
                dst[j++] = '2';
                dst[j++] = '0';
            }
        } else {
            // Other characters - percent encode
            if (j + 3 < dst_size) {
                dst[j++] = '%';
                dst[j++] = hex[c >> 4];
                dst[j++] = hex[c & 0x0F];
            }
        }
    }
    dst[j] = '\0';
}

static weather_condition_t parse_weather_condition(const char *main)
{
    if (strcmp(main, "Clear") == 0) {
        return WEATHER_CLEAR;
    } else if (strcmp(main, "Clouds") == 0) {
        return WEATHER_CLOUDS;
    } else if (strcmp(main, "Rain") == 0) {
        return WEATHER_RAIN;
    } else if (strcmp(main, "Drizzle") == 0) {
        return WEATHER_DRIZZLE;
    } else if (strcmp(main, "Thunderstorm") == 0) {
        return WEATHER_THUNDERSTORM;
    } else if (strcmp(main, "Snow") == 0) {
        return WEATHER_SNOW;
    } else if (strstr(main, "Mist") != NULL || strstr(main, "Fog") != NULL) {
        return WEATHER_MIST;
    }
    return WEATHER_UNKNOWN;
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            // Accept both chunked and non-chunked responses
            if (http_response_len + evt->data_len < MAX_HTTP_RECV_BUFFER) {
                memcpy(http_response_buffer + http_response_len, evt->data, evt->data_len);
                http_response_len += evt->data_len;
                ESP_LOGD(TAG, "Received %d bytes, total: %d", evt->data_len, http_response_len);
            } else {
                ESP_LOGW(TAG, "HTTP response buffer overflow! Current: %d, incoming: %d", 
                         http_response_len, evt->data_len);
            }
            break;
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER: %s: %s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH, total received: %d bytes", http_response_len);
            break;
        default:
            break;
    }
    return ESP_OK;
}

static esp_err_t fetch_current_weather(void)
{
    char url[512];
    char encoded_city[128];
    
    // URL encode the city name to handle spaces and special characters
    url_encode(CONFIG_OWM_CITY, encoded_city, sizeof(encoded_city));
    
    snprintf(url, sizeof(url),
             "http://api.openweathermap.org/data/2.5/weather?q=%s,%s&appid=%s&units=metric",
             encoded_city, CONFIG_OWM_COUNTRY_CODE, CONFIG_OWM_API_KEY);

    ESP_LOGI(TAG, "Fetching weather from: %s", url);

    http_response_len = 0;
    memset(http_response_buffer, 0, sizeof(http_response_buffer));

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }
    
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "Current weather HTTP status: %d", status_code);
        if (status_code == 200) {
            http_response_buffer[http_response_len] = '\0';
            
            // Debug: log first 300 chars of JSON response
            ESP_LOGI(TAG, "JSON (current): %.300s", http_response_buffer);
            
            cJSON *root = cJSON_Parse(http_response_buffer);
            if (root != NULL) {
                cJSON *main_obj = cJSON_GetObjectItem(root, "main");
                cJSON *weather = cJSON_GetObjectItem(root, "weather");
                cJSON *dt = cJSON_GetObjectItem(root, "dt");
                
                if (main_obj && weather && cJSON_IsArray(weather)) {
                    cJSON *weather_item = cJSON_GetArrayItem(weather, 0);
                    cJSON *temp = cJSON_GetObjectItem(main_obj, "temp");
                    cJSON *weather_main = cJSON_GetObjectItem(weather_item, "main");
                    cJSON *description = cJSON_GetObjectItem(weather_item, "description");
                    
                    if (temp && weather_main && description) {
                        current_weather.temp = temp->valuedouble;
                        current_weather.condition = parse_weather_condition(weather_main->valuestring);
                        strncpy(current_weather.description, description->valuestring, 
                                sizeof(current_weather.description) - 1);
                        current_weather.dt = dt ? dt->valueint : 0;
                        
                        // Debug: log temperature as integer to avoid float printf issues
                        int temp_int = (int)current_weather.temp;
                        ESP_LOGI(TAG, "Current weather: %dC, %s", 
                                 temp_int, current_weather.description);
                        err = ESP_OK;
                    } else {
                        ESP_LOGE(TAG, "Failed to parse weather data - temp:%p, weather_main:%p, desc:%p", 
                                 temp, weather_main, description);
                    }
                }
                cJSON_Delete(root);
            }
        } else {
            ESP_LOGE(TAG, "HTTP GET request failed with status code: %d", status_code);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}

static esp_err_t fetch_forecast(void)
{
    char url[512];
    char encoded_city[128];
    
    // URL encode the city name to handle spaces and special characters
    url_encode(CONFIG_OWM_CITY, encoded_city, sizeof(encoded_city));
    
    // Request only 8 items (24 hours) to reduce JSON size and parsing complexity
    snprintf(url, sizeof(url),
             "http://api.openweathermap.org/data/2.5/forecast?q=%s,%s&appid=%s&units=metric&cnt=8",
             encoded_city, CONFIG_OWM_COUNTRY_CODE, CONFIG_OWM_API_KEY);

    ESP_LOGI(TAG, "Fetching forecast from: %s", url);

    http_response_len = 0;
    memset(http_response_buffer, 0, sizeof(http_response_buffer));

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }
    
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        int content_length = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG, "Forecast HTTP status: %d, Content-Length: %d, Received: %d", 
                 status_code, content_length, http_response_len);
        if (status_code == 200) {
            http_response_buffer[http_response_len] = '\0';
            
            // Debug: log response length and first/last chars of JSON
            ESP_LOGI(TAG, "JSON (forecast) - Length: %d bytes", http_response_len);
            ESP_LOGI(TAG, "JSON (forecast) - First 200 chars: %.200s", http_response_buffer);
            
            // Show last 200 chars to see if JSON is complete
            int start = (http_response_len > 200) ? http_response_len - 200 : 0;
            ESP_LOGI(TAG, "JSON (forecast) - Last 200 chars: %s", &http_response_buffer[start]);
            
            cJSON *root = cJSON_Parse(http_response_buffer);
            
            if (root == NULL) {
                const char *error_ptr = cJSON_GetErrorPtr();
                if (error_ptr != NULL) {
                    ESP_LOGE(TAG, "JSON parse error before: %.50s", error_ptr);
                }
            }
            
            if (root != NULL) {
                cJSON *list = cJSON_GetObjectItem(root, "list");

                if (list && cJSON_IsArray(list)) {
                    int array_size = cJSON_GetArraySize(list);
                    
                    // With cnt=8 (24 hours, 3-hour intervals):
                    // Index 2 = +6h, Index 4 = +12h, Index 6 = +18h
                    // We'll use indices 2 and 4 for tomorrow's forecast (morning and afternoon)
                    int indices[2] = {2, 4};  // ~6 hours and ~12 hours ahead

                    for (int i = 0; i < 2 && i < array_size; i++) {
                        int idx = (indices[i] < array_size) ? indices[i] : i + 1;

                        cJSON *forecast_item = cJSON_GetArrayItem(list, idx);

                        if (forecast_item) {
                            cJSON *main_obj = cJSON_GetObjectItem(forecast_item, "main");
                            cJSON *weather = cJSON_GetObjectItem(forecast_item, "weather");
                            cJSON *dt = cJSON_GetObjectItem(forecast_item, "dt");

                            if (main_obj && weather && cJSON_IsArray(weather)) {
                                cJSON *weather_item = cJSON_GetArrayItem(weather, 0);
                                cJSON *temp = cJSON_GetObjectItem(main_obj, "temp");
                                cJSON *weather_main = cJSON_GetObjectItem(weather_item, "main");
                                cJSON *description = cJSON_GetObjectItem(weather_item, "description");
                                
                                if (temp && weather_main && description) {
                                    forecast_data[i].temp = temp->valuedouble;
                                    forecast_data[i].condition = parse_weather_condition(weather_main->valuestring);
                                    strncpy(forecast_data[i].description, description->valuestring,
                                            sizeof(forecast_data[i].description) - 1);
                                    forecast_data[i].dt = dt ? dt->valueint : 0;
                                } else {
                                    ESP_LOGE(TAG, "Failed parsing forecast day %d - missing fields", i);
                                }
                            } else {
                                ESP_LOGE(TAG, "Failed parsing forecast day %d - main_obj or weather invalid", i);
                            }
                        } else {
                            ESP_LOGE(TAG, "Failed to get forecast item at index %d", idx);
                        }
                    }
                    err = ESP_OK;
                } else {
                    ESP_LOGE(TAG, "Forecast list not found or not an array");
                    err = ESP_FAIL;
                }
                cJSON_Delete(root);
            } else {
                ESP_LOGE(TAG, "Failed to parse forecast JSON");
                err = ESP_FAIL;
            }
        } else {
            ESP_LOGE(TAG, "HTTP GET forecast failed with status code: %d", status_code);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET forecast failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}

static void weather_update_task(void *pvParameters)
{
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));  // Wait 10 seconds before first update
        
        ESP_LOGI(TAG, "Updating weather data...");
        
        esp_err_t err1 = fetch_current_weather();
        vTaskDelay(pdMS_TO_TICKS(1000));  // Small delay between requests
        esp_err_t err2 = fetch_forecast();
        
        if (err1 == ESP_OK && err2 == ESP_OK) {
            weather_data_valid = true;
            ESP_LOGI(TAG, "Weather data updated successfully");
        } else {
            weather_data_valid = false;
            ESP_LOGW(TAG, "Failed to update weather data");
        }
        
        // Wait for next update
        vTaskDelay(pdMS_TO_TICKS(CONFIG_OWM_UPDATE_INTERVAL * 60 * 1000));
    }
}

void weather_api_init(void)
{
    memset(&current_weather, 0, sizeof(current_weather));
    memset(forecast_data, 0, sizeof(forecast_data));
    
    // Create task with larger stack (8KB) to handle JSON parsing
    xTaskCreate(weather_update_task, "weather_update", 8192, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Weather API initialized");
}

esp_err_t weather_get_current(weather_forecast_t *forecast)
{
    if (!weather_data_valid || forecast == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    memcpy(forecast, &current_weather, sizeof(weather_forecast_t));
    return ESP_OK;
}

esp_err_t weather_get_forecast(int day, weather_forecast_t *forecast)
{
    if (!weather_data_valid || forecast == NULL || day < 0 || day > 2) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Day 0 = current weather, Days 1-2 = forecast data
    if (day == 0) {
        memcpy(forecast, &current_weather, sizeof(weather_forecast_t));
    } else {
        memcpy(forecast, &forecast_data[day - 1], sizeof(weather_forecast_t));
    }
    return ESP_OK;
}

bool weather_is_valid(void)
{
    return weather_data_valid;
}
