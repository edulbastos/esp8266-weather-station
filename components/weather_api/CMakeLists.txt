idf_component_register(SRCS "weather_api.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_client json lwip)
