# Spooldex Humidity Tracker — Hub Firmware

ESP32-C6 firmware that acts as a BLE-to-HTTP gateway for Xiaomi LYWSD03MMC temperature/humidity sensors running pvvx or BTHome v2 firmware.

## Features

✅ **BLE Passive Scanning** — Receives pvvx custom format and BTHome v2 advertisements  
✅ **HTTP REST Push** — Forwards sensor data to REST API with Bearer auth  
✅ **SSD1306 OLED Display** — Shows live sensor readings, cycles every 3 seconds  
✅ **WiFi Captive Portal** — First-boot setup via browser (192.168.4.1)  
✅ **NVS Configuration** — Persistent WiFi/API credentials  
✅ **OTA Updates** — HTTPS firmware updates from configurable URL  
✅ **Status LED** — Visual feedback for hub state  
✅ **Auto-reconnect** — WiFi with exponential backoff  
✅ **Watchdog Timer** — System reset on hang  
✅ **NTP Time Sync** — Accurate timestamps  
✅ **Health Reporting** — Uptime, free heap, WiFi RSSI to REST endpoint  

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
   - API URL (e.g., `http://localhost:3000/api/humidity/readings`)
   - API Key (optional, for Bearer authentication)
   - Hub name (optional, default: `spooldex-hub`)
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

## REST API Endpoints

### Sensor Readings

**POST** `<api_url>` (default: `http://localhost:3000/api/humidity/readings`)

Headers:
```
Content-Type: application/json
Authorization: Bearer <api_key>
```

Payload:
```json
{
  "hub_id": "spooldex-hub",
  "readings": [
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
  ]
}
```

### Hub Health

**POST** `<api_url_base>/health` (e.g., `http://localhost:3000/api/humidity/health`)

Headers: Same as above

Payload:
```json
{
  "hub_id": "spooldex-hub",
  "uptime": 3600,
  "free_heap": 123456,
  "sensors": 4,
  "wifi_rssi": -65
}
```

## OTA Updates

### Manual Trigger

Set OTA URL in provisioning portal, then trigger update via your backend or web UI.

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
| `SPOOLDEX_URL` | `http://localhost:3000/api/humidity/readings` | REST API endpoint |
| `API_KEY` | "" | Bearer token for API auth |
| `PUSH_INTERVAL_S` | 30 | Publish interval |
| `DISPLAY_ENABLED` | Yes | Enable OLED display |
| `MAX_SENSORS` | 16 | Max tracked sensors |
| `STATUS_LED_PIN` | 8 | Status LED GPIO |
| `WATCHDOG_TIMEOUT_S` | 60 | Watchdog timeout |
| `NTP_SERVER` | `pool.ntp.org` | NTP server |
| `HEALTH_REPORT_INTERVAL_S` | 300 | Health push interval |

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

### HTTP push failing

- Verify API URL is reachable from hub's network
- Check API key if authentication is required
- Look for HTTP status codes in serial logs

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

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                     ESP32-C6 Hub                        │
│                                                         │
│  ┌──────────┐  BLE Ads   ┌────────────┐                │
│  │  BLE     │ ───────▶   │ Sensor DB  │                │
│  │ Scanner  │            │ (in-memory)│                │
│  └──────────┘            └─────┬──────┘                │
│                                 │                        │
│                                 ▼                        │
│                          ┌─────────────┐                │
│                          │  Publish    │                │
│                          │  Task       │                │
│                          └──────┬──────┘                │
│                                 │                        │
│                                 ▼                        │
│                          ┌─────────────┐                │
│                          │ HTTP Push   │                │
│                          │ (REST API)  │                │
│                          └──────┬──────┘                │
│                                 │                        │
└─────────────────────────────────┼───────────────────────┘
                                  │
                                  ▼
                         ┌──────────────────┐
                         │  Spooldex API    │
                         │  (Node.js +      │
                         │   PostgreSQL)    │
                         └──────────────────┘
```

## License

MIT — See [LICENSE](../LICENSE)

## Credits

- **pvvx** — Custom BLE firmware for LYWSD03MMC
- **BTHome** — Open BLE sensor standard
- **ESP-IDF** — Espressif IoT Development Framework
