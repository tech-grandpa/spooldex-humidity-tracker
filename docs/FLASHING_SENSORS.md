# Flashing Xiaomi LYWSD03MMC Sensors

## What You Need

- Xiaomi LYWSD03MMC sensor (with battery inserted)
- A computer with Chrome or Edge browser (Web Bluetooth required)
- The sensor within ~2 meters of your computer

## Steps

### 1. Open the Web Flasher

Go to: **https://pvvx.github.io/ATC_MiThermometer/TelinkMiFlasher.html**

### 2. Connect to Your Sensor

1. Click **Connect**
2. In the browser popup, look for a device named `LYWSD03MMC`
3. Select it and click **Pair**

> If you have multiple sensors, do them one at a time. You can identify them by the MAC address shown on the back of the sensor.

### 3. Activate the Sensor

1. Click **Do Activation** — this unlocks the sensor for custom firmware
2. Wait for the process to complete (a few seconds)

### 4. Flash Custom Firmware

1. Click **Custom Firmware** to load the latest pvvx firmware
2. Click **Start Flashing**
3. Wait for flashing to complete (~30 seconds)
4. The sensor will reboot automatically

### 5. Configure the Sensor

After reboot, reconnect to the sensor (click **Connect** again):

1. **Advertising type:** Select `pvvx custom` (recommended for this project)
   - Alternative: `BTHome v2` if you also want Home Assistant compatibility
2. **Advertising interval:** Set to `10` seconds (best battery life for filament monitoring)
3. **Device name:** Rename to something meaningful:
   - `DryBox-1`, `DryBox-2`, `Storage-A`, `PrintRoom`, etc.
4. **Comfort parameters** (optional):
   - Set humidity comfort max to `40` (alerts on the sensor display when above 40% RH)
   - Set temperature comfort range as desired

5. Click **Send Config** to save

### 6. Verify

The sensor display should now show temperature and humidity. The hub will automatically discover it within one scan cycle (~10 seconds).

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Sensor not found in Connect dialog | Make sure Bluetooth is on, sensor is nearby, battery is fresh |
| "Activation failed" | Try removing and reinserting the battery, then retry |
| Flash stuck at 0% | Refresh the page and reconnect |
| Sensor shows wrong values after flash | Remove battery for 10s, reinsert — it recalibrates |

## Reverting to Stock Firmware

If you ever want to go back to Xiaomi's original firmware:

1. Open the web flasher
2. Connect to the sensor
3. Use the **Restore Original Firmware** option

## Multiple Sensors

Just repeat the process for each sensor. Give each one a unique name so you can identify them in the hub readings.

**Tip:** Label the physical sensors with their names (e.g., a small sticker on the back) so you know which is which when placing them in dry boxes.

## Firmware Options Comparison

| Format | Hub Compatibility | Home Assistant | Battery Impact |
|--------|------------------|----------------|----------------|
| pvvx custom | ✅ Native support | Via ESPHome | Lowest |
| BTHome v2 | ✅ Supported | ✅ Native | Low |
| ATC (original) | ✅ Supported | Via ESPHome | Low |

We recommend **pvvx custom** format for best battery life and native hub support.
