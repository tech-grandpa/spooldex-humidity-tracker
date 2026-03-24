# Build Instructions

## Prerequisites

### 1. Install ESP-IDF v5.4+

```bash
# On macOS (Homebrew)
brew install cmake ninja dfu-util

# Install ESP-IDF
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout release/v5.4
./install.sh esp32c6

# Add to your shell profile (~/.zshrc or ~/.bashrc)
alias get_idf='. ~/esp/esp-idf/export.sh'
```

### 2. Activate ESP-IDF Environment

```bash
get_idf
```

## Build & Flash

### Configure (Optional)

```bash
cd firmware
idf.py set-target esp32c6
idf.py menuconfig
```

Navigate to: **Spooldex Humidity Tracker Configuration**

### Build

```bash
idf.py build
```

### Flash to ESP32-C6

```bash
# Find serial port (usually /dev/ttyUSB0 or /dev/cu.usbserial-*)
ls /dev/tty* | grep -i usb

# Flash
idf.py -p /dev/ttyUSB0 flash

# Flash + Monitor (to see logs)
idf.py -p /dev/ttyUSB0 flash monitor
```

### Exit Monitor

Press `Ctrl+]`

## Troubleshooting

### "Permission denied" on serial port

```bash
# Linux
sudo usermod -a -G dialout $USER
# Log out and back in

# macOS — shouldn't happen, but if it does:
sudo chmod 666 /dev/cu.usbserial-*
```

### "Target mismatch"

```bash
idf.py set-target esp32c6
idf.py fullclean
idf.py build
```

### Out of memory during build

```bash
# Reduce parallel jobs
idf.py build -j2
```

## Build Artifacts

After successful build:

- **Firmware binary:** `build/spooldex-humidity-tracker.bin`
- **Bootloader:** `build/bootloader/bootloader.bin`
- **Partition table:** `build/partition_table/partition-table.bin`
- **ELF for debugging:** `build/spooldex-humidity-tracker.elf`

## CI/CD

For automated builds (GitHub Actions, etc.):

```yaml
- name: Setup ESP-IDF
  uses: espressif/esp-idf-ci-action@v1
  with:
    esp_idf_version: v5.4
    target: esp32c6

- name: Build
  run: |
    cd firmware
    idf.py build
```

## OTA Binary

For OTA updates, upload: `build/spooldex-humidity-tracker.bin`

Minimum server requirements:
- HTTPS recommended
- Must be accessible from hub's network
- Set `Content-Type: application/octet-stream`
