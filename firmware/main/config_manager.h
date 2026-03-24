#pragma once

#include <stdbool.h>

#define CONFIG_SSID_MAX_LEN 32
#define CONFIG_PASS_MAX_LEN 64
#define CONFIG_URL_MAX_LEN  128
#define CONFIG_NAME_MAX_LEN 32
#define CONFIG_KEY_MAX_LEN  64

/**
 * Hub configuration stored in NVS.
 */
typedef struct {
    char wifi_ssid[CONFIG_SSID_MAX_LEN];
    char wifi_password[CONFIG_PASS_MAX_LEN];
    char api_url[CONFIG_URL_MAX_LEN];
    char api_key[CONFIG_KEY_MAX_LEN];
    char hub_name[CONFIG_NAME_MAX_LEN];
    char ota_url[CONFIG_URL_MAX_LEN];
    bool configured;
} hub_config_t;

/**
 * Initialize config manager and load from NVS.
 * Falls back to Kconfig defaults if NVS is empty.
 */
void config_manager_init(void);

/**
 * Get current configuration.
 */
const hub_config_t *config_get(void);

/**
 * Check if hub has been configured (has WiFi credentials in NVS).
 */
bool config_is_provisioned(void);

/**
 * Save configuration to NVS.
 */
esp_err_t config_save(const hub_config_t *cfg);

/**
 * Reset configuration to defaults and erase NVS.
 */
void config_reset(void);
