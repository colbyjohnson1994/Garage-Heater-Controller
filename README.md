# Garage Heater Controller (PlatformIO)

ESP12S / ESP8266 garage heater controller with modern web UI, outdoor weather, weekly schedule, login, and ArduinoOTA.

## Build & Upload

```bash
# USB upload
pio run -t upload

# Monitor
pio device monitor

# OTA (set IP in platformio.ini first)
pio run -t upload --upload-port 192.168.1.XXX
```

## Project Layout

- `platformio.ini` — board, libs, OTA settings
- `src/main.cpp` — main sketch
- `include/` — headers (pages, config, helpers)

## Default Login

- Username: `admin`
- Password: `heater`

## OTA Password

`heaterota` (change in `src/main.cpp` or via build flag)

## Libraries

ArduinoJson is declared in `platformio.ini` and will be installed automatically.
