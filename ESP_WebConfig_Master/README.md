# Modern Garage Heater Controller (ESP12S / ESP8266)

Updated version of the original ESP_WebConfig with:

- **Modern dark-mode web interface** with responsive cards
- **Outdoor weather** from Open-Meteo (no API key required)
- **Daily scheduling** (on/off times, overnight support)
- JSON API for status
- Improved NTP time

## Features

### Dashboard (`/`)
- Large indoor temperature display
- Outdoor weather (temp, humidity, condition)
- Heater enable toggle
- Setpoint and hysteresis adjustment
- Live status badge (HEATING / IDLE)

### Schedule (`/schedule.html`)
- Enable/disable daily schedule
- Set turn-on and turn-off times
- Supports overnight windows (e.g. 22:00–06:00)

### Network & Location (`/config.html`)
- WiFi SSID / password
- DHCP or static IP
- Latitude / Longitude for weather (default: Rapid City, SD)

### Info (`/info.html`)
- MAC, IP, free heap, uptime

## Hardware (unchanged)
- ESP12S (ESP8266)
- Thermistor on A0 (Rs = 150kΩ)
- Relay on GPIO5
- GPIO0 for long-press factory reset

## Libraries Required
Install via Arduino Library Manager:
- **ArduinoJson** (v6 recommended)
- ESP8266 core (WiFi, WebServer, HTTPClient already included)

## Setup
1. Open `ESP_WebConfig_Master.ino` in Arduino IDE
2. Select board: **Generic ESP8266 Module** or NodeMCU
3. Upload
4. Connect to AP "Garage Heater" (password: 12345678) or use your WiFi
5. Open the IP shown in Serial Monitor
6. Set your location (lat/long) in Settings for accurate weather
7. Configure schedule and setpoints

## Weather
Uses free Open-Meteo API:
```
http://api.open-meteo.com/v1/forecast?latitude=XX&longitude=YY&current=temperature_2m,relative_humidity_2m,weather_code&temperature_unit=fahrenheit
```
Updates every 15 minutes when WiFi is connected.

## Notes
- Schedule only allows heating between the configured times when enabled.
- Long-press GPIO0 (5s) resets to defaults and restarts.
- All settings stored in EEPROM.
- Units in Fahrenheit to match original project.

## Original
Based on John Lassen's ESP_WebConfig (2015).
