# 🌡️ Spooldex Humidity Tracker

**Open-source temperature & humidity monitoring for 3D printer filament storage.**

A low-cost sensor network for tracking environmental conditions in your filament dry boxes, storage containers, and print rooms — designed to integrate with [Spooldex](https://github.com/tech-grandpa/spooldex).

## Overview

The system consists of two parts:

1. **Hub** — An ESP32-C6 gateway that receives sensor data via BLE (and Zigbee in the future) and pushes readings to your server over WiFi
2. **Sensor Nodes** — Cheap off-the-shelf Xiaomi LYWSD03MMC temperature/humidity sensors running custom open-source firmware

**Cost:** Under €25 for a hub + 5 sensors. No cloud subscription required.

## Architecture

```
┌─────────────┐     BLE      ┌─────────────┐     WiFi/HTTP     ┌──────────┐
│  Xiaomi     │  broadcast   │  ESP32-C6   │    ──────────►   │ Spooldex │
│  Sensor #1  │ ──────────►  │    Hub      │                   │  Server  │
└─────────────┘              │             │     REST API       │          │
┌─────────────┐              │  - BLE scan │    (Bearer auth)   │          │
│  Sensor #2  │ ──────────►  │  - WiFi up  │    ──────────►   │          │
└─────────────┘              │  - Display  │                   └──────────┘
┌─────────────┐              │  - LED      │
│  Sensor #N  │ ──────────►  │             │
└─────────────┘              └─────────────┘
      ...                     (future: Zigbee
                               via 802.15.4)
```

### Protocol Roadmap

| Protocol | Status | Notes |
|----------|--------|-------|
| BLE (pvvx) | ✅ MVP | Passive scanning, no pairing needed |
| Zigbee | 🔮 Planned | ESP32-C6 has native 802.15.4 radio |
| Thread/Matter | 🔮 Future | Same 802.15.4 hardware supports it |

The ESP32-C6 was chosen specifically because it supports WiFi + BLE + 802.15.4 (Zigbee/Thread) on a single chip — so the same hardware can support all three protocols via firmware updates.

## Hardware

### Shopping List

#### Hub (one needed)

| Part | Description | Price (AliExpress) |
|------|-------------|-------------------|
| **Seeed XIAO ESP32-C6** (recommended) | Tiny (21×17.5mm), USB-C, WiFi 6 + BLE 5 + 802.15.4, built-in LiPo charger | ~€5 |
| SSD1306 0.96" OLED (optional) | I2C display for local readings | ~€1.50 |
| USB-C cable | Power + initial flashing | ~€1 |
| **Total** | | **~€7** |

> **Alternative:** ESP32-C6-DevKitC-1 (~€3) — larger board, more GPIOs, no battery charger. Works with the same firmware.

#### Sensor Nodes (as many as you need)

| Part | Description | Price (AliExpress) |
|------|-------------|-------------------|
| Xiaomi LYWSD03MMC | BLE temp/humidity sensor with display | ~€3-4 each |
| CR2032 battery (spare) | Included with sensor, spares are cheap | ~€0.20 each |

**Example setup — 5 dry boxes:** 1 hub (€7) + 5 sensors (€20) = **~€27 total**

### AliExpress Search Terms

- Hub: `"Seeed XIAO ESP32-C6"` or `"ESP32-C6 development board"`
- Sensors: `"Xiaomi LYWSD03MMC"` or `"Xiaomi Mijia thermometer 2"`
- Display: `"SSD1306 0.96 OLED I2C"` (get the 4-pin I2C version)

### Hub Wiring (with optional OLED)

**Seeed XIAO ESP32-C6:**
```
XIAO ESP32-C6      SSD1306 OLED
──────────────      ────────────
3.3V  ──────────    VCC
GND   ──────────    GND
GPIO6 (D4/SDA) ──  SDA
GPIO7 (D5/SCL) ──  SCL
```

**ESP32-C6-DevKitC-1:**
```
ESP32-C6          SSD1306 OLED
────────          ────────────
3.3V  ──────────  VCC
GND   ──────────  GND
GPIO6 ──────────  SDA
GPIO7 ──────────  SCL
```

No other wiring needed — the sensors are standalone battery-powered units.

## Sensor Firmware (pvvx)

The Xiaomi LYWSD03MMC sensors need custom firmware to broadcast their readings openly via BLE. The [pvvx firmware](https://github.com/pvvx/ATC_MiThermometer) is the community standard — it's stable, well-maintained, and flashed entirely from your browser.

### Flashing Steps

1. Open the [pvvx web flasher](https://pvvx.github.io/ATC_MiThermometer/TelinkMiFlasher.html) in Chrome/Edge
2. Click **Connect** — select your Xiaomi sensor (it advertises as `LYWSD03MMC`)
3. Click **Do Activation** to unlock the sensor
4. Click **Custom Firmware** → **Start Flashing**
5. After reboot, reconnect and configure:
   - **Advertising type:** pvvx custom (recommended) or BTHome
   - **Advertising interval:** 2.5s (good balance of freshness vs battery)
   - **Comfort parameters:** Set humidity thresholds (e.g., alert > 50% RH for filament)
6. Rename the sensor (e.g., `DryBox-1`, `Storage-A`) for easy identification

**No soldering. No tools. Takes about 2 minutes per sensor.**

### Battery Life

| Advertising Interval | Estimated Battery Life (CR2032) |
|---------------------|-------------------------------|
| 2.5 seconds | ~8-10 months |
| 5 seconds | ~12-14 months |
| 10 seconds | ~18+ months |

For filament monitoring, 10-second intervals are perfectly fine — humidity doesn't change that fast.

## Hub Firmware

The hub firmware is an ESP-IDF project for the ESP32-C6. It passively scans for BLE advertisements from pvvx-firmware sensors and forwards readings via WiFi.

### Features

- **Passive BLE scanning** — no pairing, just listens for broadcasts
- **Auto-discovery** — new sensors appear automatically
- **REST API push** — readings pushed directly to Spooldex HTTP API with Bearer auth
- **OLED display** — shows sensor count, latest readings, WiFi status
- **OTA updates** — flash new firmware over WiFi
- **Low resource** — runs on the C6's single RISC-V core, plenty of headroom

### Building & Flashing

```bash
# Prerequisites: ESP-IDF v5.4+
# https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/get-started/

# Clone this repo
git clone https://github.com/tech-grandpa/spooldex-humidity-tracker.git
cd spooldex-humidity-tracker/firmware

# Set target
idf.py set-target esp32c6

# Configure WiFi and API
idf.py menuconfig
# → Spooldex Humidity Tracker Configuration
#   → WiFi SSID / Password
#   → Spooldex API URL (e.g., http://localhost:3000/api/humidity/readings)
#   → API Key (optional, for Bearer authentication)
#   → Push interval (default: 30 seconds)

# Build and flash
idf.py build flash monitor
```

### Configuration (menuconfig)

| Setting | Default | Description |
|---------|---------|-------------|
| WiFi SSID | — | Your WiFi network |
| WiFi Password | — | WiFi password |
| Spooldex URL | `http://localhost:3000/api/humidity/readings` | REST API endpoint |
| API Key | — | Bearer token for authentication |
| BLE Scan Window | 5000 ms | How long to scan per cycle |
| BLE Scan Interval | 10000 ms | Time between scan cycles |
| Push Interval | 30 s | How often to send data to server |
| Display Enabled | true | Enable SSD1306 OLED |
| Health Report Interval | 300 s | How often to send health metrics |

### REST API Format

The hub pushes readings via HTTP POST:

**Sensor Readings:**
```
POST /api/humidity/readings
Content-Type: application/json
Authorization: Bearer <api_key>

{
  "hub_id": "spooldex-hub",
  "readings": [
    {
      "mac": "A4:C1:38:XX:XX:XX",
      "name": "DryBox-1",
      "temperature": 23.5,
      "humidity": 42.1,
      "battery_pct": 87,
      "battery_mv": 2950,
      "rssi": -65,
      "timestamp": 1711234567
    }
  ]
}
```

**Hub Health:**
```
POST /api/humidity/health
Content-Type: application/json
Authorization: Bearer <api_key>

{
  "hub_id": "spooldex-hub",
  "uptime": 3600,
  "free_heap": 123456,
  "sensors": 4,
  "wifi_rssi": -65
}
```

## Spooldex Integration

Once the hub is reporting data, Spooldex can:

- **Link sensors to spool locations** — assign `DryBox-1` to a specific storage container or dry box
- **Track humidity history** — chart humidity over time per storage location
- **Alert on thresholds** — notify when humidity exceeds safe levels (> 15-20% for hygroscopic filaments like Nylon/PETG, > 40% for PLA)
- **Correlate with print quality** — compare humidity exposure with print outcomes

### Filament Humidity Guidelines

| Material | Max Safe Humidity | Notes |
|----------|------------------|-------|
| PLA | 40-50% RH | Tolerant, but prints better dry |
| PETG | 30-40% RH | Absorbs moisture, gets stringy |
| Nylon/PA | 15-20% RH | Very hygroscopic, must be dry |
| TPU | 30-40% RH | Gets bubbly when wet |
| ABS | 40-50% RH | Relatively tolerant |
| ASA | 30-40% RH | Similar to PETG |
| PVA | 10-15% RH | Extremely hygroscopic |
| PC | 20-30% RH | Absorbs slowly but significantly |

## Project Structure

```
spooldex-humidity-tracker/
├── firmware/              # ESP-IDF hub firmware (ESP32-C6)
│   ├── main/
│   │   ├── main.c         # Application entry point
│   │   ├── ble_scanner.c  # BLE passive scanner for pvvx sensors
│   │   ├── ble_scanner.h
│   │   ├── http_push.c    # REST API push with Bearer auth
│   │   ├── http_push.h
│   │   ├── display.c      # SSD1306 OLED driver
│   │   ├── display.h
│   │   ├── sensor_db.c    # In-memory sensor registry
│   │   ├── sensor_db.h
│   │   ├── config_manager.c  # NVS configuration storage
│   │   ├── config_manager.h
│   │   ├── wifi_provision.c  # Captive portal for setup
│   │   ├── wifi_provision.h
│   │   └── Kconfig        # menuconfig options
│   ├── CMakeLists.txt
│   ├── sdkconfig.defaults
│   └── partitions.csv
├── docs/
│   ├── FLASHING_SENSORS.md
│   ├── WIRING.md
│   └── ALIEXPRESS_LINKS.md
├── hardware/
│   └── case/              # 3D printable hub enclosure (future)
├── LICENSE
└── README.md
```

## Development Roadmap

- [x] Project setup and documentation
- [x] Hub firmware — BLE passive scanner (pvvx format)
- [x] Hub firmware — WiFi + REST API push
- [x] Hub firmware — OLED display
- [x] Hub firmware — REST API push with Bearer auth
- [x] Hub firmware — OTA updates
- [x] Hub firmware — Web config portal (WiFi AP mode for initial setup)
- [ ] Spooldex integration — humidity data model + API endpoints
- [ ] Spooldex integration — sensor assignment UI
- [ ] Spooldex integration — humidity history charts
- [ ] Zigbee coordinator support (ESP32-C6 802.15.4)
- [ ] 3D printable hub enclosure
- [ ] PCB design for compact hub (optional)

## Contributing

This is an open-source project under the [tech-grandpa](https://github.com/tech-grandpa) org. Contributions welcome — especially:

- Testing with different sensor hardware
- Zigbee coordinator implementation
- 3D printable enclosures
- Spooldex frontend integration

## License

MIT
