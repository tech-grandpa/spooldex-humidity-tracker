#include "display.h"
#include "sensor_db.h"
#include "esp_log.h"

static const char *TAG = "display";

/*
 * TODO: Implement SSD1306 I2C driver.
 *
 * Recommended: Use the `esp_lcd` component with SSD1306 panel driver,
 * or a lightweight library like `ssd1306` from ESP Component Registry.
 *
 * Display layout (128x64 OLED):
 *
 * ┌──────────────────────┐
 * │ Spooldex Hub   5 sns │  <- header: title + sensor count
 * │──────────────────────│
 * │ DryBox-1       42.1% │  <- sensor name + humidity
 * │ 23.5°C     🔋 87%   │  <- temperature + battery
 * │──────────────────────│
 * │ DryBox-2       38.7% │  <- next sensor (cycle every 3s)
 * │ 22.1°C     🔋 92%   │
 * └──────────────────────┘
 *
 * If no sensors found, show "Scanning..." with animated dots.
 */

#ifdef CONFIG_DISPLAY_ENABLED
void display_init(void)
{
    ESP_LOGI(TAG, "Display init (SDA=%d, SCL=%d)",
             CONFIG_DISPLAY_SDA_PIN, CONFIG_DISPLAY_SCL_PIN);
    // TODO: Initialize I2C and SSD1306
    ESP_LOGW(TAG, "Display driver not yet implemented — readings available via MQTT/HTTP");
}

void display_update(void)
{
    // TODO: Render current sensor readings to OLED
    // Cycle through sensors every 3 seconds
}
#else
void display_init(void)
{
    ESP_LOGI(TAG, "Display disabled");
}

void display_update(void)
{
    // No-op
}
#endif
