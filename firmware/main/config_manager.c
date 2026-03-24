#include "config_manager.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

static const char *TAG = "config";
static const char *NVS_NAMESPACE = "hub_config";

static hub_config_t config;

void config_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing configuration manager");

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);

    if (err == ESP_OK) {
        size_t len;

        // Try to read from NVS
        len = sizeof(config.wifi_ssid);
        if (nvs_get_str(handle, "wifi_ssid", config.wifi_ssid, &len) != ESP_OK) {
            goto use_defaults;
        }

        len = sizeof(config.wifi_password);
        if (nvs_get_str(handle, "wifi_pass", config.wifi_password, &len) != ESP_OK) {
            goto use_defaults;
        }

        len = sizeof(config.api_url);
        if (nvs_get_str(handle, "api_url", config.api_url, &len) != ESP_OK) {
            strlcpy(config.api_url, CONFIG_SPOOLDEX_URL, sizeof(config.api_url));
        }

        len = sizeof(config.api_key);
        if (nvs_get_str(handle, "api_key", config.api_key, &len) != ESP_OK) {
            strlcpy(config.api_key, CONFIG_API_KEY, sizeof(config.api_key));
        }

        len = sizeof(config.hub_name);
        if (nvs_get_str(handle, "hub_name", config.hub_name, &len) != ESP_OK) {
            strlcpy(config.hub_name, "spooldex-hub", sizeof(config.hub_name));
        }

        len = sizeof(config.ota_url);
        if (nvs_get_str(handle, "ota_url", config.ota_url, &len) != ESP_OK) {
            config.ota_url[0] = '\0';
        }

        config.configured = true;
        nvs_close(handle);

        ESP_LOGI(TAG, "Loaded config from NVS: SSID=%s, API=%s, Hub=%s",
                 config.wifi_ssid, config.api_url, config.hub_name);
        return;
    }

use_defaults:
    if (err == ESP_OK) nvs_close(handle);

    // Fall back to Kconfig defaults
    ESP_LOGW(TAG, "No valid config in NVS, using Kconfig defaults");
    strlcpy(config.wifi_ssid, CONFIG_WIFI_SSID, sizeof(config.wifi_ssid));
    strlcpy(config.wifi_password, CONFIG_WIFI_PASSWORD, sizeof(config.wifi_password));
    strlcpy(config.api_url, CONFIG_SPOOLDEX_URL, sizeof(config.api_url));
    strlcpy(config.api_key, CONFIG_API_KEY, sizeof(config.api_key));
    strlcpy(config.hub_name, "spooldex-hub", sizeof(config.hub_name));
    config.ota_url[0] = '\0';

    // Mark as configured if Kconfig has WiFi credentials
    config.configured = (strlen(config.wifi_ssid) > 0);
}

const hub_config_t *config_get(void)
{
    return &config;
}

bool config_is_provisioned(void)
{
    return config.configured && strlen(config.wifi_ssid) > 0;
}

esp_err_t config_save(const hub_config_t *cfg)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    nvs_set_str(handle, "wifi_ssid", cfg->wifi_ssid);
    nvs_set_str(handle, "wifi_pass", cfg->wifi_password);
    nvs_set_str(handle, "api_url", cfg->api_url);
    nvs_set_str(handle, "api_key", cfg->api_key);
    nvs_set_str(handle, "hub_name", cfg->hub_name);
    nvs_set_str(handle, "ota_url", cfg->ota_url);

    err = nvs_commit(handle);
    nvs_close(handle);

    if (err == ESP_OK) {
        memcpy(&config, cfg, sizeof(config));
        config.configured = true;
        ESP_LOGI(TAG, "Configuration saved to NVS");
    } else {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
    }

    return err;
}

void config_reset(void)
{
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK) {
        nvs_erase_all(handle);
        nvs_commit(handle);
        nvs_close(handle);
    }
    ESP_LOGI(TAG, "Configuration reset");
}
