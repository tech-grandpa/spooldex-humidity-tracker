# AliExpress Shopping Guide

## Search Terms

AliExpress listings change frequently. Use these search terms to find the right parts:

### Hub Board

**Search:** `ESP32-C6-DevKitC-1` or `ESP32-C6 development board`

**What to look for:**
- ESP32-C6 chip (NOT C3 or S3 — only C6 has 802.15.4 for future Zigbee)
- USB-C connector
- DevKitC-1 form factor (standard breadboard-friendly)
- ~€2-4

**Alternatives:**
- WeAct ESP32-C6 mini (~€2, smaller form factor)
- Seeed XIAO ESP32-C6 (~€5, very compact, USB-C)

### Temperature/Humidity Sensors

**Search:** `Xiaomi LYWSD03MMC` or `Xiaomi Mijia thermometer 2` or `Xiaomi Bluetooth thermometer LCD`

**What to look for:**
- Model: LYWSD03MMC (square, ~43mm, with LCD display)
- Comes with CR2032 battery
- ~€3-4 each, often cheaper in packs of 3-5

**⚠️ Avoid:**
- LYWSD02 (the older, larger model — different hardware)
- LYWSD03MMC **v2** — some newer revisions may have different chips. The pvvx firmware page lists compatible hardware revisions.

### OLED Display (Optional)

**Search:** `SSD1306 0.96 OLED I2C` or `0.96 inch OLED display module I2C`

**What to look for:**
- 0.96" size
- I2C interface (4 pins: VCC, GND, SDA, SCL)
- SSD1306 driver chip
- Blue, white, or blue/yellow dual-color
- ~€1-2

**⚠️ Make sure** it's the 4-pin I2C version, NOT the 7-pin SPI version.

### Example Order (5 dry boxes)

| Item | Qty | ~Price |
|------|-----|--------|
| ESP32-C6-DevKitC-1 | 1 | €3 |
| Xiaomi LYWSD03MMC | 5 | €18 |
| SSD1306 0.96" OLED I2C | 1 | €1.50 |
| CR2032 batteries (spare pack) | 10 | €2 |
| Dupont wires (F-F) | 1 pack | €1 |
| **Total** | | **~€25** |

### Shipping Tips

- **Combined shipping:** Buy from the same store when possible
- **Delivery time:** Standard AliExpress shipping to Germany is typically 2-4 weeks
- **Bulk discount:** Xiaomi sensors are often cheaper in 3-packs or 5-packs
