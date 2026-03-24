# Hub Wiring Guide

## Minimal Setup (No Display)

Just plug the ESP32-C6 into USB power. That's it. No wiring needed — BLE scanning and WiFi work out of the box.

## With SSD1306 OLED Display

### Parts

- Seeed XIAO ESP32-C6 (recommended) or ESP32-C6-DevKitC-1
- SSD1306 0.96" OLED display (I2C, 4-pin version)
- 4x Dupont jumper wires (female-to-female)

### Connections (Seeed XIAO ESP32-C6)

```
XIAO ESP32-C6      SSD1306 OLED
──────────────      ────────────
3.3V  ──────────    VCC
GND   ──────────    GND
GPIO6 (D4/SDA) ──  SDA
GPIO7 (D5/SCL) ──  SCL
```

### Connections (ESP32-C6-DevKitC-1)

```
ESP32-C6          SSD1306 OLED
────────          ────────────
3.3V  ──────────  VCC
GND   ──────────  GND
GPIO6 ──────────  SDA
GPIO7 ──────────  SCL
```

### Pin Reference

The default I2C pins can be changed in `menuconfig` → Spooldex Humidity Tracker Configuration:

| Function | Default GPIO | Configurable |
|----------|-------------|-------------|
| I2C SDA  | GPIO 6      | Yes |
| I2C SCL  | GPIO 7      | Yes |

### Display Orientation

The display should be oriented with the pins at the top or bottom. The firmware auto-detects the I2C address (typically 0x3C for SSD1306).

### Photos

*(TODO: Add wiring photos)*

## Power Options

| Method | Notes |
|--------|-------|
| USB-C (recommended) | Simple, reliable, 5V from any charger |
| 3.3V direct | Advanced — bypass USB, power the 3.3V pin directly |
| Battery | Possible with 18650 + regulator, but the hub doesn't need to be portable |

For filament monitoring, the hub sits in one place — USB power is ideal.
