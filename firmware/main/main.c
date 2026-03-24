/**
 * Spooldex Humidity Tracker — Hub Firmware
 *
 * ESP32-C6 gateway that receives BLE advertisements from pvvx-firmware
 * Xiaomi LYWSD03MMC sensors and forwards readings via MQTT and/or HTTP.
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

#include "ble_scanner.h"
#include "mqtt_client.h"
#include "http_push.h"
#include "display.h"
#include "sensor_db.h"

static const char *TAG = "main";

/* ── WiFi ──────────────────────────────────────────────────────── */

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi disconnected, reconnecting...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi connected, IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

static void wifi_init(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "WiFi connecting to %s...", CONFIG_WIFI_SSID);
}

/* ── Main Loop ─────────────────────────────────────────────────── */

static void publish_task(void *arg)
{
    const TickType_t interval = pdMS_TO_TICKS(CONFIG_PUSH_INTERVAL_S * 1000);

    while (1) {
        vTaskDelay(interval);

        // Prune sensors not seen in 5 minutes
        sensor_db_prune(300);

        // Publish via MQTT
        mqtt_publish_readings();

        // Publish via HTTP (if enabled)
        http_push_readings();

        // Update display
        display_update();

        int count = sensor_db_count();
        if (count > 0) {
            ESP_LOGI(TAG, "Tracking %d sensor(s), next push in %ds",
                     count, CONFIG_PUSH_INTERVAL_S);
        }
    }
}

/* ── Entry Point ───────────────────────────────────────────────── */

void app_main(void)
{
    ESP_LOGI(TAG, "╔══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║  Spooldex Humidity Tracker Hub       ║");
    ESP_LOGI(TAG, "║  github.com/tech-grandpa             ║");
    ESP_LOGI(TAG, "╚══════════════════════════════════════╝");

    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // Initialize subsystems
    sensor_db_init();
    wifi_init();
    display_init();

    // Wait a moment for WiFi to connect before starting MQTT
    vTaskDelay(pdMS_TO_TICKS(3000));

    mqtt_client_init();
    http_push_init();

    // Start BLE scanning
    ble_scanner_init();

    // Give NimBLE a moment to initialize
    vTaskDelay(pdMS_TO_TICKS(1000));
    ble_scanner_start();

    // Start periodic publish task
    xTaskCreate(publish_task, "publish", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Hub running — scanning for sensors...");
}
