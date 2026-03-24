#pragma once

/**
 * Initialize OTA update subsystem.
 * Sets up MQTT command handler for remote OTA triggers.
 */
void ota_init(void);

/**
 * Check for and perform OTA update from configured URL.
 * Blocks until update completes or fails.
 * Returns ESP_OK if successful, error otherwise.
 */
esp_err_t ota_perform_update(const char *url);

/**
 * Background task that periodically checks for updates.
 * Call from main() if you want automatic update checks.
 */
void ota_check_task_start(void);
