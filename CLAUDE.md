# CLAUDE.md — Spooldex Humidity Tracker

## Project Overview

Open-source temperature & humidity monitoring for 3D printer filament storage.
A BLE-to-HTTP gateway hub (ESP32-C6) that receives sensor data from Xiaomi LYWSD03MMC
sensors and pushes readings to the Spooldex API.

**Repo:** github.com/tech-grandpa/spooldex-humidity-tracker
**Parent project:** github.com/tech-grandpa/spooldex

## Architecture

- **Hub:** Seeed XIAO ESP32-C6 (WiFi 6 + BLE 5 + 802.15.4)
- **Sensors:** Xiaomi LYWSD03MMC with pvvx or BTHome v2 custom firmware
- **Protocol:** BLE passive scanning → HTTP REST push (Bearer auth)
- **Framework:** ESP-IDF v5.4+
- **Language:** C

No MQTT. REST-only by design — simpler, fits the Next.js/Prisma stack, no broker needed.

## Repository Structure

```
firmware/
├── main/
│   ├── main.c              # Entry point, WiFi init, task creation
│   ├── ble_scanner.c/.h    # BLE passive scan (pvvx + BTHome v2)
│   ├── sensor_db.c/.h      # In-memory sensor storage, pruning
│   ├── http_push.c/.h      # REST API push (readings + health)
│   ├── display.c/.h        # SSD1306 OLED display driver (I2C)
│   ├── config_manager.c/.h # NVS persistent config (WiFi, API key, etc.)
│   ├── wifi_provision.c/.h # Captive portal for first-boot setup
│   ├── ota_update.c/.h     # HTTPS OTA firmware updates
│   ├── status_led.c/.h     # Status LED patterns
│   ├── version.h           # Firmware version
│   └── Kconfig             # Menuconfig options
├── CMakeLists.txt
├── sdkconfig.defaults
├── partitions.csv
├── BUILD.md
├── CHANGELOG.md
└── README.md
docs/
├── ALIEXPRESS_LINKS.md
├── WIRING.md
└── FLASHING_SENSORS.md
hardware/
└── case/                   # 3D-printable enclosure (planned)
```

## Key Configuration (Kconfig)

| Option | Default | Purpose |
|--------|---------|---------|
| `PUSH_INTERVAL_S` | 300 (5 min) | How often to push readings to API |
| `BLE_SCAN_WINDOW_MS` | 5000 | Active BLE scan duration per cycle |
| `BLE_SCAN_INTERVAL_MS` | 10000 | Time between BLE scan cycles |
| `DISPLAY_ENABLED` | yes | Enable SSD1306 OLED (I2C) |
| `DISPLAY_SDA_PIN` | 6 | I2C SDA GPIO |
| `DISPLAY_SCL_PIN` | 7 | I2C SCL GPIO |
| `STATUS_LED_PIN` | 8 | Status LED GPIO |
| `MAX_SENSORS` | 16 | Max tracked sensors in memory |
| `HEALTH_REPORT_INTERVAL_S` | 300 | Health metrics push interval |
| `WATCHDOG_TIMEOUT_S` | 60 | Task watchdog timeout |

## Current Display: SSD1306 OLED (I2C)

- 128×64 pixels, 0.96"
- Connected via I2C (SDA=GPIO6, SCL=GPIO7)
- Cycles through sensor readings every 3 seconds
- Adequate for basic status but too small for trend data

## Planned: E-Ink Display Upgrade

### Decision

Replace the SSD1306 OLED with a **WeAct 2.9" BW e-paper module** (SSD1680 controller).

### Rationale

- E-ink is ideal for slow-changing data (5-minute refresh cadence)
- 2.9" (296×128) gives enough room for trend graphs + sensor list
- Lower power than always-on OLED
- Better glanceability — dashboard stays visible even when unpowered
- Black/white version preferred over red variant (faster refresh, better partial updates)

### E-Ink Wiring Plan (XIAO ESP32-C6 → WeAct 2.9" BW)

| ESP32-C6 | E-Paper Pin | Function |
|----------|-------------|----------|
| 3V3 | VCC | Power |
| GND | GND | Ground |
| GPIO6 | DIN/MOSI | SPI data |
| GPIO4 | CLK/SCK | SPI clock |
| GPIO7 | CS | Chip select |
| GPIO1 | DC | Data/command |
| GPIO2 | RST | Reset |
| GPIO3 | BUSY | Busy signal |

If EN pin exists: tie to 3V3 (or spare GPIO for power gating).

### E-Ink Screen Layout (296×128 landscape)

```
┌─────────────────────────────────────────────────────────┐
│  Humidity Overview            WiFi OK · API Synced      │
│  5 min refresh · updated 14:05                          │
├──────────────┬──────────────────────────────────────────┤
│              │  24h Humidity Trend                      │
│  Avg: 42%    │  ┌──────────────────────────────────┐   │
│  ↓ 3% / 24h │  │  ╲                               │   │
│              │  │    ╲___                           │   │
│  21.8°C      │  │        ╲___                      │   │
│  Stable      │  │             ╲___                  │   │
│              │  └──────────────────────────────────┘   │
├──────────────┴─────────┬───────────────┬───────────────┤
│  Rack A    41%  21.4°  │ Dry Box 36%   │ AMS     48%  │
│  ↘ drying             │ → stable      │ ↗ rising     │
└────────────────────────┴───────────────┴───────────────┘
```

### E-Ink Refresh Strategy

- Partial refresh every **5 minutes** (synced with push interval)
- Full refresh every **1 hour** (or every 12 partial updates) to clear ghosting
- No rapid cycling — e-ink is not suited for live page flipping
- Refresh triggered after successful data push

### Firmware Changes Needed for E-Ink

1. **Display abstraction layer** — generic `display_init()`, `display_render_dashboard()`, `display_sleep()`
2. **New Kconfig options** — `DISPLAY_TYPE` (oled/epaper), SPI pin config (MOSI, SCK, CS, DC, RST, BUSY)
3. **SSD1680 driver** — SPI-based e-paper controller driver
4. **Trend buffer** — Rolling 24h history in RAM (~288 points at 5-min intervals)
5. **Dashboard renderer** — Layout engine for the e-ink screen (header, metrics, chart, sensor tiles)
6. **Remove OLED code path** — Or keep behind `DISPLAY_TYPE` config for backwards compatibility

### UI Mock Files

Three design iterations were created and rendered:
- `eink-iter-1-minimal` — Clean dashboard, strong hierarchy (recommended for firmware)
- `eink-iter-2-industrial` — Technical/instrument-panel style
- `eink-iter-3-graph-first` — Maximizes trend chart area

## Build & Flash

```bash
cd firmware
idf.py set-target esp32c6
idf.py menuconfig    # Configure WiFi, API URL, display options
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## API Integration

The hub pushes to these Spooldex API endpoints:
- `POST /api/humidity/readings` — Sensor data (temp, humidity, battery, RSSI)
- `POST /api/humidity/health` — Hub health metrics (uptime, heap, WiFi RSSI)

Auth: Bearer token (`shk_` prefix, SHA-256 hashed in Spooldex DB).

## Conventions

- All firmware code is C (ESP-IDF, not Arduino)
- Pin assignments are configurable via Kconfig menuconfig
- Sensor data is kept in-memory only (no flash wear)
- Sensors not seen in 5 minutes are pruned from the database
- GPG signing not available on this machine — use `git -c commit.gpgsign=false commit`
