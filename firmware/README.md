# Spooldex Humidity Tracker — Hub Firmware

ESP32-C6 firmware that acts as a BLE-to-MQTT/HTTP gateway for Xiaomi LYWSD03MMC temperature/humidity sensors running pvvx or BTHome v2 firmware.

## Features

✅ **BLE Passive Scanning** — Receives pvvx custom format and BTHome v2 advertisements  
✅ **MQTT Publishing** — Forwards sensor data to MQTT broker with JSON payloads  
✅ **HTTP REST Push** — Optional HTTP POST to REST API  
✅ **SSD1306 OLED Display** — Shows live sensor readings, cycles every 3 seconds  
✅ **WiFi Captive Portal** — First-boot setup via browser (192.168.4.1)  
✅ **NVS Configuration** — Persistent WiFi/MQTT credentials  
✅ **OTA Updates** — HTTPS firmware updates from configurable URL  
✅ **Status LED** — Visual feedback for hub state  
✅ **Auto-reconnect** — WiFi and MQTT with exponential backoff  
✅ **Watchdog Timer** — System reset on hang  
✅ **NTP Time Sync** — Accurate timestamps  
✅ **Health Reporting** — Uptime, free heap, WiFi RSSI, error counts  

## Hardware Requirements

- **MCU:** ESP32-C6 (RISC-V)
- **Display:** SSD1306 128x64 OLED (I2C, optional)
- **LED:** Single GPIO for status indication
- **Sensors:** Xiaomi LYWSD03MMC with pvvx or BTHome v2 BLE firmware

### Pin Configuration

| Function | GPIO | Configurable via Kconfig |
|----------|------|--------------------------|
| Display SDA | 6 | `CONFIG_DISPLAY_SDA_PIN` |
| Display SCL | 7 | `CONFIG_DISPLAY_SCL_PIN` |
| Status LED | 8 | `CONFIG_STATUS_LED_PIN` |

## Building & Flashing

### Prerequisites

- ESP-IDF v5.4 or later
- ESP32-C6 toolchain

### Build

```bash
cd firmware
idf.py set-target esp32c6
idf.py menuconfig  # Optional: configure defaults
idf.py build
```

### Flash

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

## First Boot Setup

1. Power on the ESP32-C6
2. Status LED will blink **fast** (AP mode)
3. Connect to WiFi network: **Spooldex-Hub-Setup** (no password)
4. Browser will redirect to captive portal, or visit: **http://192.168.4.1**
5. Enter:
   - WiFi SSID & password
   - MQTT broker URL (e.g., `mqtt://10.10.10.123:1883`)
   - Hub name (optional)
   - OTA update URL (optional)
6. Click **Save & Reboot**
7. Hub will connect to WiFi and start scanning

## Status LED Patterns

| Pattern | Meaning |
|---------|---------|
| Fast blink (200ms) | AP config mode |
| Slow blink (1s) | Connecting to WiFi |
| Solid on | Connected, scanning |
| Double blink | Sensor data received |

## MQTT Topics

All topics are prefixed with `CONFIG_MQTT_TOPIC_PREFIX` (default: `spooldex/humidity`).

### Sensor Data

Topic: `<prefix>/sensors/<MAC>`

Payload:
```json
{
  "mac": "A4:C1:38:XX:XX:XX",
  "name": "DryBox-1",
  "temperature": 23.45,
  "humidity": 42.10,
  "battery_pct": 87,
  "battery_mv": 2950,
  "rssi": -65,
  "timestamp": 1738012345
}
```

### Hub Status

| Topic | Description |
|-------|-------------|
| `<prefix>/hub/status` | `online` / `offline` (LWT, retained) |
| `<prefix>/hub/sensor_count` | Number of active sensors |
| `<prefix>/hub/uptime` | Hub uptime in seconds |
| `<prefix>/hub/free_heap` | Free heap memory in bytes |
| `<prefix>/hub/wifi_rssi` | WiFi signal strength (dBm) |

## OTA Updates

### Manual Trigger

Set OTA URL in provisioning portal, then:
```bash
mosquitto_pub -t spooldex/humidity/hub/ota -m "update"
```

### Automatic Checks

Uncomment in `main.c`:
```c
ota_check_task_start();  // Checks daily
```

## Configuration (Kconfig)

Run `idf.py menuconfig` and navigate to **Spooldex Humidity Tracker Configuration**:

| Option | Default | Description |
|--------|---------|-------------|
| `WIFI_SSID` | "" | WiFi network name (fallback) |
| `WIFI_PASSWORD` | "" | WiFi password (fallback) |
| `MQTT_BROKER_URL` | `mqtt://localhost:1883` | MQTT broker |
| `MQTT_TOPIC_PREFIX` | `spooldex/humidity` | Base MQTT topic |
| `PUSH_INTERVAL_S` | 30 | Publish interval |
| `HTTP_PUSH_ENABLED` | No | Enable HTTP REST push |
| `DISPLAY_ENABLED` | Yes | Enable OLED display |
| `MAX_SENSORS` | 16 | Max tracked sensors |
| `STATUS_LED_PIN` | 8 | Status LED GPIO |
| `WATCHDOG_TIMEOUT_S` | 60 | Watchdog timeout |
| `NTP_SERVER` | `pool.ntp.org` | NTP server |

## Supported Sensor Formats

### pvvx Custom Format

Service UUID: `0x181A` (Environmental Sensing)

17-byte payload with MAC, temp, humidity, battery.

### BTHome v2

Service UUID: `0xFCD2`

Object IDs:
- `0x01` — Battery %
- `0x02` — Temperature (0.01°C)
- `0x03` — Humidity (0.01%)
- `0x0C` — Voltage (mV)

## Troubleshooting

### Hub won't connect to WiFi

- Check SSID/password in provisioning portal
- LED should blink slowly during connection attempts
- Check serial output: `idf.py monitor`

### MQTT not connecting

- Verify broker URL format: `mqtt://IP:PORT`
- Check broker is reachable from hub's network
- Look for MQTT error count in health reports

### Display shows "Scanning..." forever

- Ensure sensors are nearby and powered
- Check sensors are running pvvx or BTHome v2 firmware
- Verify BLE is enabled in `sdkconfig`

### Reset to factory

Hold GPIO 0 during boot (if implemented), or:
```bash
idf.py erase-flash
idf.py flash
```

## Memory Usage

ESP32-C6 has limited RAM. To optimize:

- Reduce `MAX_SENSORS` if you have fewer sensors
- Disable `DISPLAY_ENABLED` if not using OLED
- Set `CONFIG_COMPILER_OPTIMIZATION_SIZE=y`

## License

MIT — See [LICENSE](../LICENSE)

## Credits

- **pvvx** — Custom BLE firmware for LYWSD03MMC
- **BTHome** — Open BLE sensor standard
- **ESP-IDF** — Espressif IoT Development Framework
