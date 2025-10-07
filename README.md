# ESP8266 Weather Station with OLED Display

A weather station based on ESP8266 with SSD1306 OLED display, DHT22 sensor, and OpenWeatherMap API integration.

## Features

- **SSD1306 OLED Display** (128x64) connected via I2C (SDA=GPIO12, SCL=GPIO14)
- **DHT22 Sensor** (GPIO4) for local temperature and humidity readings
- **OpenWeatherMap Integration** for weather forecast (current + 2 future periods)
- **NTP Time Synchronization** for accurate time display
- **WiFi Signal Indicator** with simple bar-style icon
- **Weather Icons** (sun, clouds, rain, thunderstorm, snow, mist)
- **Day of Week Display** for each forecast period (Sun, Mon, Tue, Wed, Thu, Fri, Sat)
- **KConfig-based Configuration** for all critical parameters

## Hardware Requirements

- ESP8266 (NodeMCU, Wemos D1 Mini, etc.)
- SSD1306 OLED Display 128x64 (I2C)
- DHT22 Sensor Module (AM2302) with built-in pull-up resistor

## Wiring

| Component | GPIO | Physical Pin |
|-----------|------|--------------|
| SSD1306 SDA | GPIO12 (D6) | - |
| SSD1306 SCL | GPIO14 (D5) | - |
| DHT22 Data | GPIO4 (D2) | - |
| DHT22 VCC | 3.3V | - |
| DHT22 GND | GND | - |

## Environment Setup

### Prerequisites

1. ESP8266_RTOS_SDK installed
2. ESP8266 toolchain configured

### Set environment variables

```bash
export IDF_PATH=/path/to/ESP8266_RTOS_SDK
export PATH="$IDF_PATH/tools:$PATH"
```

## Build and Flash

### 1. Configure the project

```bash
cd /path/to/esp8266_weather_oled
idf.py menuconfig
```

Configure the following parameters under **Weather Station Configuration**:

#### WiFi Configuration
- **WiFi SSID**: Your WiFi network name
- **WiFi Password**: Network password
- **Maximum retry attempts**: Connection retry attempts (default: 5)

#### OpenWeatherMap API Configuration
- **OpenWeatherMap API Key**: Your API key (get it at https://openweathermap.org/api)
- **City name**: City name (e.g., "New York", "London", "Tokyo")
- **Country code**: Country code (e.g., "US", "GB", "JP")
- **Weather update interval**: Update interval in minutes (default: 30)

#### Time Configuration
- **NTP Server**: NTP server address (default: "pool.ntp.org")
- **Timezone**: Timezone string (e.g., "UTC-5", "UTC+9")
- **Time sync interval**: Synchronization interval in hours (default: 24)

#### DHT22 Sensor Configuration
- **DHT22 GPIO Pin**: DHT22 data pin (default: 4)
- **DHT22 read interval**: Reading interval in seconds (default: 60)

#### Display Configuration
- **SSD1306 SDA GPIO Pin**: Display SDA pin (default: 12)
- **SSD1306 SCL GPIO Pin**: Display SCL pin (default: 14)
- **SSD1306 I2C Address**: I2C address (default: 0x3C)
- **Display update interval**: Update interval in seconds (default: 5)

### 2. Build

```bash
idf.py build
```

### 3. Flash to ESP8266

```bash
idf.py -p /dev/ttyUSB0 flash
```

Replace `/dev/ttyUSB0` with your correct serial port.

### 4. Serial Monitor

```bash
idf.py -p /dev/ttyUSB0 monitor
```

## Usage

After flashing and booting:

1. ESP8266 automatically connects to the configured WiFi
2. Synchronizes time with NTP server
3. Starts reading DHT22 sensor
4. Fetches weather data from OpenWeatherMap API
5. Displays on OLED:
   - **Top line**: Current time (left) | Indoor temperature from DHT22 (center) | WiFi indicator (right)
   - **Left side**: Current weather icon (large) with temperature and day of week
   - **Right side**: Two future forecast periods with icons, temperatures, and days of week

## Display Layout

```
┌────────────────────────────────────┐
│ 14:35      23.5°C       [WiFi ▂▄▆█]│
│────────────────────────────────────│
│                                    │
│  ☀️              ☁️      🌧️      │
│  28°C            24°C    22°C      │
│  Sun             Mon     Tue       │
│                                    │
└────────────────────────────────────┘
```

**Features:**
- Large current weather icon (32x32 pixels) on the left
- Two smaller forecast icons (16x16 pixels) on the right
- WiFi indicator shows signal bars when connected, nothing when disconnected
- Day names displayed below each temperature (3-letter abbreviations)
- Improved cloud icon with better shape and visibility

## Troubleshooting

### Display not working
- Check I2C connections (SDA and SCL)
- Verify display I2C address (0x3C or 0x3D)
- Test with I2C scanner

### DHT22 not reading data
- Check data pin connection
- Verify module has built-in pull-up resistor
- Wait 2 seconds after boot for stabilization

### No weather data
- Check WiFi connection
- Verify API key is correct
- Check logs with `idf.py monitor`
- Free API tier has request limits

### Incorrect time
- Check internet connection
- Configure timezone correctly in menuconfig
- Wait for NTP synchronization (may take up to 30 seconds)

## Project Structure

```
esp8266_weather_oled/
├── CMakeLists.txt              # Root CMake file
├── Kconfig.projbuild           # Project configuration
├── README.md                   # This file
├── main/
│   ├── CMakeLists.txt
│   └── esp8266_weather_oled.c  # Main application
└── components/
    ├── wifi_manager/           # WiFi management
    ├── time_manager/           # NTP synchronization
    ├── dht22/                  # DHT22 driver
    ├── weather_api/            # OpenWeatherMap client
    └── ssd1306/                # OLED display driver
        ├── ssd1306.c           # Display initialization and layout
        ├── ssd1306_draw.c      # Drawing functions and icons
        └── ssd1306_fonts.c     # Font definitions
```

## Component Details

### WiFi Manager
- Handles WiFi connection and reconnection
- Configurable retry attempts
- Status monitoring

### Time Manager
- NTP-based time synchronization
- Timezone support
- Periodic re-synchronization

### DHT22 Driver
- Temperature and humidity readings
- Configurable read interval
- Error handling and retry logic

### Weather API
- OpenWeatherMap 5-day/3-hour forecast API
- Parses current weather and 2 future periods
- Weather condition icons mapping
- Automatic periodic updates

### SSD1306 Display Driver
- I2C communication
- Custom font with lowercase letters support
- Weather icons (normal and large size)
- WiFi signal indicator
- Drawing primitives (pixels, lines, rectangles)

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Feel free to open issues or pull requests.

## Additional Resources

- [ESP8266 RTOS SDK Documentation](https://docs.espressif.com/projects/esp8266-rtos-sdk/)
- [OpenWeatherMap API Documentation](https://openweathermap.org/api)
- [SSD1306 Datasheet](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf)
- [DHT22 Datasheet](https://www.sparkfun.com/datasheets/Sensors/Temperature/DHT22.pdf)
