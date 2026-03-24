#include "sensor_db.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "sensor_db";

static sensor_entry_t sensors[CONFIG_MAX_SENSORS];
static SemaphoreHandle_t db_mutex;

void sensor_db_init(void)
{
    memset(sensors, 0, sizeof(sensors));
    db_mutex = xSemaphoreCreateMutex();
    ESP_LOGI(TAG, "Sensor database initialized (max %d sensors)", CONFIG_MAX_SENSORS);
}

static void mac_to_str(const uint8_t mac[6], char *out)
{
    snprintf(out, SENSOR_MAC_STR_LEN, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static int find_sensor(const uint8_t mac[6])
{
    for (int i = 0; i < CONFIG_MAX_SENSORS; i++) {
        if (sensors[i].active && memcmp(sensors[i].mac, mac, 6) == 0) {
            return i;
        }
    }
    return -1;
}

static int find_free_slot(void)
{
    for (int i = 0; i < CONFIG_MAX_SENSORS; i++) {
        if (!sensors[i].active) {
            return i;
        }
    }
    return -1;
}

int sensor_db_update(const uint8_t mac[6], const char *name,
                     float temperature, float humidity,
                     uint8_t battery_pct, uint16_t battery_mv,
                     int8_t rssi)
{
    xSemaphoreTake(db_mutex, portMAX_DELAY);

    int idx = find_sensor(mac);
    if (idx < 0) {
        idx = find_free_slot();
        if (idx < 0) {
            ESP_LOGW(TAG, "Sensor database full, ignoring new sensor");
            xSemaphoreGive(db_mutex);
            return -1;
        }
        memcpy(sensors[idx].mac, mac, 6);
        mac_to_str(mac, sensors[idx].mac_str);
        sensors[idx].active = true;
        ESP_LOGI(TAG, "New sensor discovered: %s (%s)", name ? name : "unknown", sensors[idx].mac_str);
    }

    sensors[idx].temperature = temperature;
    sensors[idx].humidity = humidity;
    sensors[idx].battery_pct = battery_pct;
    sensors[idx].battery_mv = battery_mv;
    sensors[idx].rssi = rssi;
    sensors[idx].last_seen = (uint32_t)time(NULL);

    if (name && name[0] != '\0') {
        strncpy(sensors[idx].name, name, SENSOR_NAME_MAX_LEN - 1);
        sensors[idx].name[SENSOR_NAME_MAX_LEN - 1] = '\0';
    }

    xSemaphoreGive(db_mutex);
    return idx;
}

const sensor_entry_t *sensor_db_get(int index)
{
    if (index < 0 || index >= CONFIG_MAX_SENSORS || !sensors[index].active) {
        return NULL;
    }
    return &sensors[index];
}

int sensor_db_count(void)
{
    int count = 0;
    xSemaphoreTake(db_mutex, portMAX_DELAY);
    for (int i = 0; i < CONFIG_MAX_SENSORS; i++) {
        if (sensors[i].active) count++;
    }
    xSemaphoreGive(db_mutex);
    return count;
}

void sensor_db_prune(uint32_t timeout_s)
{
    uint32_t now = (uint32_t)time(NULL);
    xSemaphoreTake(db_mutex, portMAX_DELAY);
    for (int i = 0; i < CONFIG_MAX_SENSORS; i++) {
        if (sensors[i].active && (now - sensors[i].last_seen) > timeout_s) {
            ESP_LOGW(TAG, "Pruning stale sensor: %s (%s)",
                     sensors[i].name, sensors[i].mac_str);
            sensors[i].active = false;
        }
    }
    xSemaphoreGive(db_mutex);
}
