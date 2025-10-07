# ESP8266 Weather Station Project Architecture

## Overview

This document describes the software architecture of the ESP8266 weather station.

## Component Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                     esp8266_weather_oled                     │
│                      (Main Application)                      │
└──────────────┬──────────────────────────────────────────────┘
               │
               │ Initializes and coordinates
               │
    ┌──────────┴────────────────────────────────────┐
    │                                                │
    ▼                                                ▼
┌──────────────────┐                    ┌─────────────────────┐
│  wifi_manager    │◄───────────────────┤   time_manager      │
│                  │   Depends on       │                     │
│ - WiFi Connect   │                    │ - NTP Sync          │
│ - Reconnect      │                    │ - Timezone          │
│ - Status         │                    │ - Local time        │
└────────┬─────────┘                    └──────────┬──────────┘
         │                                         │
         │ Used by                                 │ Used by
         │                                         │
         ▼                                         ▼
┌──────────────────┐                    ┌─────────────────────┐
│  weather_api     │                    │      ssd1306        │
│                  │                    │                     │
│ - HTTP Client    │                    │ - I2C Driver        │
│ - JSON Parser    │◄───────────────────┤ - Rendering         │
│ - Data cache     │   Displays data    │ - UI Interface      │
└──────────────────┘                    └──────────┬──────────┘
                                                   │
                                                   │ Displays data
                                                   │
                                        ┌──────────▼──────────┐
                                        │       dht22         │
                                        │                     │
                                        │ - Sensor reading    │
                                        │ - Validation        │
                                        │ - Cache             │
                                        └─────────────────────┘
```

## Components

### 1. main/esp8266_weather_oled.c

**Responsibility**: Main application and orchestration

**Functions**:
- `app_main()`: Entry point
- Initializes NVS
- Initializes all components in correct order
- Main loop (minimal, delegated to tasks)

**Dependencies**:
- All components

### 2. components/wifi_manager

**Responsibility**: WiFi connection management

**Public APIs**:
- `wifi_manager_init()`: Initialize and connect to WiFi
- `wifi_is_connected()`: Check connection status
- `wifi_get_event_group()`: Return event group for synchronization

**Features**:
- Event handler for WiFi events
- Automatic reconnection with configurable retry
- Event groups for state notification

**KConfig Settings**:
- `CONFIG_WIFI_SSID`
- `CONFIG_WIFI_PASSWORD`
- `CONFIG_WIFI_MAXIMUM_RETRY`

### 3. components/time_manager

**Responsibility**: Time synchronization and management

**Public APIs**:
- `time_manager_init()`: Initialize SNTP
- `time_manager_get_time()`: Get current time
- `time_is_synced()`: Check if time is synchronized

**Features**:
- SNTP synchronization
- Configurable timezone support
- Synchronization notification callback
- Automatic periodic resync

**KConfig Settings**:
- `CONFIG_NTP_SERVER`
- `CONFIG_TIMEZONE`
- `CONFIG_TIME_UPDATE_INTERVAL`

### 4. components/dht22

**Responsibility**: DHT22 sensor interface

**Public APIs**:
- `dht22_init()`: Initialize sensor and reading task
- `dht22_read()`: Read temperature and humidity
- `dht22_get_temperature()`: Return last temperature
- `dht22_get_humidity()`: Return last humidity
- `dht22_is_valid()`: Check if data is valid

**Features**:
- Custom 1-wire communication
- Dedicated task for periodic reading
- Checksum validation
- Last readings cache
- Negative temperature support

**KConfig Settings**:
- `CONFIG_DHT22_GPIO`
- `CONFIG_DHT22_READ_INTERVAL`

### 5. components/weather_api

**Responsibility**: OpenWeatherMap API integration

**Public APIs**:
- `weather_api_init()`: Initialize and create update task
- `weather_get_current()`: Return current weather
- `weather_get_forecast()`: Return forecast for specific day
- `weather_is_valid()`: Check if data is valid

**Structures**:
```c
typedef enum {
    WEATHER_CLEAR,
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
    int dt;
} weather_forecast_t;
```

**Features**:
- HTTP client for API calls
- JSON parser (cJSON)
- Weather data cache
- Periodic update task
- Request timeout

**KConfig Settings**:
- `CONFIG_OWM_API_KEY`
- `CONFIG_OWM_CITY`
- `CONFIG_OWM_COUNTRY_CODE`
- `CONFIG_OWM_UPDATE_INTERVAL`

### 6. components/ssd1306

**Responsibility**: OLED display interface and rendering

**Public APIs**:

**Initialization**:
- `ssd1306_init()`: Initialize display and update task

**Basic drawing**:
- `ssd1306_clear()`: Clear buffer
- `ssd1306_display()`: Update display
- `ssd1306_draw_pixel()`: Draw pixel
- `ssd1306_draw_line()`: Draw line
- `ssd1306_draw_rect()`: Draw rectangle
- `ssd1306_fill_rect()`: Draw filled rectangle

**Text and icons**:
- `ssd1306_draw_string()`: Draw string with configurable size
- `ssd1306_draw_weather_icon()`: Draw weather icon (16x16)
- `ssd1306_draw_weather_icon_large()`: Draw large weather icon (32x32)
- `ssd1306_draw_wifi_icon()`: Draw WiFi icon

**Files**:
- `ssd1306.c`: I2C driver and main task
- `ssd1306_draw.c`: Drawing functions and icons
- `ssd1306_fonts.c`: Font definitions

**Features**:
- I2C communication
- Screen buffer in memory
- Periodic update task
- 5x7 bitmap font
- Custom 16x16 and 32x32 icons
- High-level UI interface

**Screen Layout**:
```
┌────────────────────────────────────┐
│ 14:35      23.5°C       [WiFi ▂▄▆█]│  ← Line 1 (y=2)
│────────────────────────────────────│
│                                    │
│  ☀️              ☁️      🌧️      │  ← Weather icons
│  28°C            24°C    22°C      │  ← Temperatures
│  Sun             Mon     Tue       │  ← Day names
│                                    │
└────────────────────────────────────┘
```

**KConfig Settings**:
- `CONFIG_SSD1306_SDA_GPIO`
- `CONFIG_SSD1306_SCL_GPIO`
- `CONFIG_SSD1306_I2C_ADDR`
- `CONFIG_DISPLAY_UPDATE_INTERVAL`

## Data Flow

### 1. Boot and Initialization

```
app_main()
    ↓
NVS Init
    ↓
wifi_manager_init()
    ↓
[Wait for WiFi connection]
    ↓
time_manager_init()
    ↓
[Synchronize NTP]
    ↓
ssd1306_init()
    ↓
dht22_init()
    ↓
weather_api_init()
    ↓
[Operating system]
```

### 2. Data Update

```
┌─────────────────┐
│  DHT22 Task     │ (every 60s)
│  Read sensor    │
│  Update cache   │
└─────────────────┘

┌─────────────────┐
│ Weather Task    │ (every 30min)
│ HTTP GET API    │
│ Parse JSON      │
│ Update cache    │
└─────────────────┘

┌─────────────────┐
│ Display Task    │ (every 5s)
│ Read all caches │
│ Render screen   │
│ Update OLED     │
└─────────────────┘
```

### 3. Rendering Flow

```
Display Task Timer (5s)
    ↓
draw_weather_screen()
    ↓
ssd1306_clear()
    ↓
time_manager_get_time() → Draw time
    ↓
wifi_is_connected() → Draw WiFi icon
    ↓
dht22_get_temperature() → Draw indoor temp
    ↓
weather_get_forecast(0,1,2) → Draw forecast
    ↓
ssd1306_display() → Update hardware
```

## FreeRTOS Tasks

| Task | Stack | Priority | Function |
|------|-------|----------|----------|
| dht22_task | 2048 | 5 | Periodic DHT22 reading |
| weather_update_task | 4096 | 5 | API update |
| display_update_task | 4096 | 5 | Display rendering |

## Communication

### I2C (SSD1306)
- Master: ESP8266
- Slave: SSD1306 (address 0x3C)
- Clock: 100kHz
- Pull-ups: Internal or on OLED module

### 1-Wire (DHT22)
- Proprietary DHT protocol
- Module has built-in pull-up resistor
- Critical timing (microseconds)

### HTTP (OpenWeatherMap)
- HTTP client (esp_http_client)
- SSL: Disabled (http://)
- Timeout: 10 seconds
- Buffer: 4KB

## Error Handling

### WiFi
- Automatic retry up to CONFIG_WIFI_MAXIMUM_RETRY
- Detailed error logs
- Event groups for synchronization

### NTP
- Retry up to 30 attempts
- Validate year > 2020 for verification
- Automatic periodic resync

### DHT22
- Checksum validation
- Valid data flag
- Continues operating with last valid value

### Weather API
- HTTP status code handling
- JSON validation
- Maintains last valid cache on error
- Request timeout

### Display
- Check pixel bounds before drawing
- I2C error handling
- Continues operating even without data

## Optimizations

### Memory
- Display buffer: 1KB
- HTTP buffer: 4KB
- Limited string buffers
- Minimal data cache

### CPU
- Tasks sleep when idle
- I2C at 100kHz (not 400kHz) for power saving
- WiFi in STA mode only
- HTTP (not HTTPS) for power saving

### Power
- Display updated only when necessary
- Sensors polled (not interrupt) but with large intervals
- WiFi maintains connection (no sleep) for always-on weather station

## Extensibility

### Add new sensors
1. Create component in `components/new_sensor/`
2. Implement reading APIs
3. Add KConfig entries
4. Integrate in `ssd1306.c` for display

### Add new screens
1. Create `draw_new_screen()` function in `ssd1306.c`
2. Implement navigation (GPIO buttons)
3. Manage current screen state

### Add new weather APIs
1. Modify `weather_api.c`
2. Adjust JSON parser
3. Keep `weather_forecast_t` structure compatible

## Debugging

### Logs
- All components use ESP_LOG
- Configurable levels via menuconfig
- Consistent tags per component

### Serial Monitor
```bash
idf.py monitor
```

### Task Analysis
- CONFIG_FREERTOS_USE_TRACE_FACILITY=y
- Can use `vTaskList()` for debugging

## References

- [ESP8266 RTOS SDK](https://docs.espressif.com/projects/esp8266-rtos-sdk/)
- [FreeRTOS Documentation](https://www.freertos.org/Documentation/)
- [I2C Protocol](https://www.nxp.com/docs/en/user-guide/UM10204.pdf)
- [OpenWeatherMap API](https://openweathermap.org/api)
