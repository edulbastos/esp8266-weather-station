# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp8266_weather_oled)

# Enable newlib nano formatting with float support for ESP8266
# This is required for printf/sprintf with %f format specifiers
target_link_libraries(${CMAKE_PROJECT_NAME}.elf "-u _printf_float")
target_link_libraries(${CMAKE_PROJECT_NAME}.elf "-u _scanf_float")
