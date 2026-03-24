/**
 * Spooldex Humidity Tracker — Hub Firmware
 *
 * ESP32-C6 gateway that receives BLE advertisements from pvvx-firmware
 * Xiaomi LYWSD03MMC sensors and forwards readings via HTTP REST API.
 *
 * Copyright (c) 2026 tech-grandpa
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_task_wdt.h"

#include "ble_scanner.h"
#include "http_push.h"
#include "display.h"
#include "sensor_db.h"
#include "config_manager.h"
#include "wifi_provision.h"
#include "status_led.h"
#include "ota_update.h"
#include "version.h"

static const char *TAG = "main";

typedef enum {
    HUB_STATE_PROVISIONING,
    HUB_STATE_WIFI_CONNECTING,
    HUB_STATE_WIFI_CONNECTED,
    HUB_STATE_RUNNING,
} hub_state_t;

static hub_state_t hub_state = HUB_STATE_WIFI_CONNECTING;
static int wifi_retry_count = 0;
static int wifi_retry_delay_s = 5;
static const int WIFI_MAX_RETRY_DELAY = 300;  // Max 5 minutes

/* ── WiFi ──────────────────────────────────────────────────────── */

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        hub_state = HUB_STATE_WIFI_CONNECTING;
        status_led_set_mode(LED_MODE_SLOW_BLINK);
        ESP_LOGI(TAG, "Connecting to WiFi...");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        hub_state = HUB_STATE_WIFI_CONNECTING;
        status_led_set_mode(LED_MODE_SLOW_BLINK);

        // Exponential backoff
        wifi_retry_count++;
        ESP_LOGW(TAG, "WiFi disconnected, retry #%d in %ds", wifi_retry_count, wifi_retry_delay_s);
        
        vTaskDelay(pdMS_TO_TICKS(wifi_retry_delay_s * 1000));
        esp_wifi_connect();

        // Increase delay for next retry (exponential backoff)
        wifi_retry_delay_s = (wifi_retry_delay_s * 2);
        if (wifi_retry_delay_s > WIFI_MAX_RETRY_DELAY) {
            wifi_retry_delay_s = WIFI_MAX_RETRY_DELAY;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "✓ WiFi connected, IP: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_retry_count = 0;
        wifi_retry_delay_s = 5;  // Reset backoff delay
        hub_state = HUB_STATE_WIFI_CONNECTED;
        status_led_set_mode(LED_MODE_SOLID);

        // Start NTP sync
        esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, CONFIG_NTP_SERVER);
        esp_sntp_init();
    }
}

static void wifi_init_sta(void)
{
    const hub_config_t *cfg = config_get();

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t wifi_config = {0};
    strlcpy((char *)wifi_config.sta.ssid, cfg->wifi_ssid, sizeof(wifi_config.sta.ssid));
    strlcpy((char *)wifi_config.sta.password, cfg->wifi_password, sizeof(wifi_config.sta.password));

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", cfg->wifi_ssid);
}

/* ── Health Reporting ──────────────────────────────────────────── */

static void health_report_task(void *arg)
{
    const TickType_t interval = pdMS_TO_TICKS(CONFIG_HEALTH_REPORT_INTERVAL_S * 1000);

    while (1) {
        vTaskDelay(interval);

        // Collect metrics
        uint64_t uptime_sec = esp_timer_get_time() / 1000000;
        uint32_t free_heap = esp_get_free_heap_size();
        int sensor_count = sensor_db_count();

        int wifi_rssi = 0;
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            wifi_rssi = ap_info.rssi;
        }

        // Push health via HTTP
        http_push_health(uptime_sec, free_heap, sensor_count, wifi_rssi);

        ESP_LOGI(TAG, "Health: uptime=%llus, free_heap=%lu, sensors=%d, wifi_rssi=%d",
                 uptime_sec, free_heap, sensor_count, wifi_rssi);

        // Pet watchdog
        esp_task_wdt_reset();
    }
}

/* ── Main Publish Loop ─────────────────────────────────────────── */

static void publish_task(void *arg)
{
    const TickType_t interval = pdMS_TO_TICKS(CONFIG_PUSH_INTERVAL_S * 1000);

    while (1) {
        vTaskDelay(interval);

        // Prune sensors not seen in 5 minutes
        sensor_db_prune(300);

        // Publish via HTTP REST API
        http_push_readings();

        // Update display
        display_update();

        int count = sensor_db_count();
        if (count > 0) {
            ESP_LOGI(TAG, "Tracking %d sensor(s), next push in %ds",
                     count, CONFIG_PUSH_INTERVAL_S);
        }

        // Pet watchdog
        esp_task_wdt_reset();
    }
}

/* ── Entry Point ───────────────────────────────────────────────── */

void app_main(void)
{
    ESP_LOGI(TAG, "╔══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  Spooldex Humidity Tracker Hub       ║");
    ESP_LOGI(TAG, "║  Version: %-27s ║", FIRMWARE_VERSION);
    ESP_LOGI(TAG, "║  github.com/tech-grandpa             ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════╝");

    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // Initialize status LED
    status_led_init();

    // Load configuration from NVS
    config_manager_init();

    // Check if provisioning is needed
    if (wifi_provision_needed()) {
        ESP_LOGW(TAG, "No WiFi configuration found, entering provisioning mode");
        hub_state = HUB_STATE_PROVISIONING;
        status_led_set_mode(LED_MODE_FAST_BLINK);
        wifi_provision_start();
        // Blocks until configured, then reboots
        return;
    }

    // Initialize subsystems
    sensor_db_init();
    display_init();

    // Connect to WiFi
    wifi_init_sta();

    // Wait for WiFi connection
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    int wait_count = 0;
    while (hub_state != HUB_STATE_WIFI_CONNECTED && wait_count < 30) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        wait_count++;
    }

    if (hub_state != HUB_STATE_WIFI_CONNECTED) {
        ESP_LOGE(TAG, "Failed to connect to WiFi, rebooting in 5s...");
        vTaskDelay(pdMS_TO_TICKS(5000));
        esp_restart();
    }

    // Initialize HTTP client
    http_push_init();

    // Initialize OTA
    ota_init();
    // Optionally start automatic OTA checks (uncomment if desired)
    // ota_check_task_start();

    // Start BLE scanning
    ble_scanner_init();
    vTaskDelay(pdMS_TO_TICKS(1000));
    ble_scanner_start();

    // Configure and enable watchdog timer
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = CONFIG_WATCHDOG_TIMEOUT_S * 1000,
        .idle_core_mask = 0,
        .trigger_panic = true,
    };
    esp_task_wdt_init(&wdt_config);
    esp_task_wdt_add(NULL);  // Add current task

    // Start background tasks
    xTaskCreate(publish_task, "publish", 4096, NULL, 5, NULL);
    xTaskCreate(health_report_task, "health", 3072, NULL, 4, NULL);

    hub_state = HUB_STATE_RUNNING;
    ESP_LOGI(TAG, "✓ Hub running — scanning for sensors...");

    // Main loop — just pet the watchdog
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        esp_task_wdt_reset();
    }
}
