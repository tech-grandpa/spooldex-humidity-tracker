#include "ota_update.h"
#include "config_manager.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ota";

#define OTA_CHECK_INTERVAL_S (3600 * 24)  // Check once per day

esp_err_t ota_perform_update(const char *url)
{
    if (!url || strlen(url) == 0) {
        ESP_LOGW(TAG, "No OTA URL configured");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Starting OTA update from: %s", url);

    esp_http_client_config_t http_config = {
        .url = url,
        .timeout_ms = 30000,
        .keep_alive_enable = true,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };

    esp_err_t ret = esp_https_ota(&ota_config);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA update successful, rebooting...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA update failed: %s", esp_err_to_name(ret));
    }

    return ret;
}

static void ota_check_task(void *arg)
{
    const TickType_t interval = pdMS_TO_TICKS(OTA_CHECK_INTERVAL_S * 1000);

    while (1) {
        vTaskDelay(interval);

        const hub_config_t *cfg = config_get();
        if (cfg && strlen(cfg->ota_url) > 0) {
            ESP_LOGI(TAG, "Checking for OTA update...");
            ota_perform_update(cfg->ota_url);
        }
    }
}

void ota_check_task_start(void)
{
    xTaskCreate(ota_check_task, "ota_check", 4096, NULL, 3, NULL);
    ESP_LOGI(TAG, "OTA update checker started");
}

void ota_init(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *boot = esp_ota_get_boot_partition();

    ESP_LOGI(TAG, "Running partition: %s at 0x%lx",
             running->label, running->address);

    if (running != boot) {
        ESP_LOGW(TAG, "Boot partition differs from running partition");
    }

    const esp_app_desc_t *app_desc = esp_app_get_description();
    ESP_LOGI(TAG, "Firmware version: %s", app_desc->version);
    ESP_LOGI(TAG, "Compile time: %s %s", app_desc->date, app_desc->time);
}
