# Changelog

All notable changes to the Spooldex Humidity Tracker Hub firmware will be documented in this file.

## [1.0.0] - 2026-03-24

### Added

- **BLE Passive Scanner** — Receives pvvx custom format and BTHome v2 advertisements from Xiaomi LYWSD03MMC sensors
- **MQTT Publisher** — Forwards sensor data to MQTT broker with JSON payloads and hub health metrics
- **HTTP REST Push** — Optional HTTP POST integration for sensor readings
- **SSD1306 OLED Display** — Live sensor readings with 3-second cycling, scanning animation, connection status
- **WiFi Captive Portal** — First-boot web UI for configuring WiFi, MQTT, and hub settings (192.168.4.1)
- **NVS Configuration Manager** — Persistent storage for WiFi/MQTT credentials with Kconfig fallback
- **OTA Updates** — HTTPS firmware updates from configurable URL with manual and periodic triggers
- **Status LED** — Visual feedback for hub state:
  - Fast blink: AP config mode
  - Slow blink: WiFi connecting
  - Solid: Connected and scanning
  - Double blink: Sensor data received
- **WiFi Auto-Reconnect** — Exponential backoff strategy (5s → 300s max delay)
- **MQTT Auto-Reconnect** — Built-in ESP-IDF MQTT client with connection resilience
- **Watchdog Timer** — System reset on hang (configurable timeout, default 60s)
- **NTP Time Sync** — Accurate timestamps for sensor readings
- **Health Reporting** — Periodic MQTT publish of hub uptime, free heap, WiFi RSSI, MQTT error count
- **BTHome v2 Support** — Parse BTHome format in addition to pvvx custom format
- **Sensor Registry** — Track up to 16 sensors (configurable) with automatic pruning of stale entries
- **Error Tracking** — MQTT error counter and health metrics

### Fixed

- **MQTT Header Conflict** — Renamed local `mqtt_client.h` to `mqtt_publisher.h` to avoid collision with ESP-IDF header

### Documentation

- Comprehensive README with features, pinout, MQTT topics, troubleshooting
- Build instructions with prerequisites and CI/CD examples
- Kconfig reference with all configuration options
- CHANGELOG for version tracking

### Technical Details

- **Target:** ESP32-C6 (RISC-V)
- **Framework:** ESP-IDF v5.4+
- **BLE Stack:** NimBLE (passive scanning)
- **Display Driver:** esp_lcd with SSD1306 panel driver
- **Components:** mqtt, esp_http_client, esp_http_server, esp_https_ota, app_update
- **Partition Table:** OTA-enabled with dual app partitions (factory + ota_0 + ota_1)
- **Optimization:** Size-optimized compilation for memory-constrained device
- **Memory Safety:** Mutex-protected sensor database, task watchdog, proper error handling

---

## [Unreleased]

### Planned

- MQTT command interface for remote control (OTA trigger, config updates, sensor reset)
- Web dashboard via HTTP server (sensor list, hub status, logs)
- MQTT TLS/SSL support for secure broker connections
- mDNS for easier hub discovery on local network
- Telegram/Discord webhook notifications for alerts
- Multi-sensor averaging and outlier detection
- Historical data logging to SD card
- Support for additional sensor formats (Mi/ATC, Ruuvi, Govee)

---

[1.0.0]: https://github.com/tech-grandpa/spooldex-humidity-tracker/releases/tag/v1.0.0
