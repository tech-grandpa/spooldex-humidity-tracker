#include <stdio.h>
#include <stdbool.h>
#include "esp_log.h"
#include "esp_event.h"
#include "mqtt_client.h"     // ESP-IDF MQTT component (from esp-mqtt)
#include "mqtt_publisher.h"  // Our header
#include "sensor_db.h"

static const char *TAG = "mqtt";

static esp_mqtt_client_handle_t client = NULL;
static bool connected = false;
static int error_count = 0;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "✓ Connected to MQTT broker");
        connected = true;
        error_count = 0;
        // Publish hub online status
        char topic[128];
        snprintf(topic, sizeof(topic), "%s/hub/status", CONFIG_MQTT_TOPIC_PREFIX);
        esp_mqtt_client_publish(client, topic, "online", 0, 1, 1);  // retained
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "Disconnected from MQTT broker (will auto-reconnect)");
        connected = false;
        break;

    case MQTT_EVENT_ERROR:
        error_count++;
        ESP_LOGE(TAG, "MQTT error (count: %d)", error_count);
        break;

    default:
        break;
    }
}

void mqtt_client_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_MQTT_BROKER_URL,
        .session.last_will = {
            .topic = CONFIG_MQTT_TOPIC_PREFIX "/hub/status",
            .msg = "offline",
            .msg_len = 7,
            .qos = 1,
            .retain = 1,
        },
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    ESP_LOGI(TAG, "MQTT client started, connecting to %s", CONFIG_MQTT_BROKER_URL);
}

void mqtt_publish_readings(void)
{
    if (!connected || !client) {
        ESP_LOGW(TAG, "MQTT not connected, skipping publish");
        return;
    }

    char topic[128];
    char payload[256];
    int count = 0;

    for (int i = 0; i < CONFIG_MAX_SENSORS; i++) {
        const sensor_entry_t *s = sensor_db_get(i);
        if (!s) continue;

        // Publish JSON payload per sensor
        snprintf(topic, sizeof(topic), "%s/sensors/%s",
                 CONFIG_MQTT_TOPIC_PREFIX, s->mac_str);

        snprintf(payload, sizeof(payload),
                 "{\"mac\":\"%s\",\"name\":\"%s\","
                 "\"temperature\":%.2f,\"humidity\":%.2f,"
                 "\"battery_pct\":%d,\"battery_mv\":%d,"
                 "\"rssi\":%d,\"timestamp\":%lu}",
                 s->mac_str, s->name,
                 s->temperature, s->humidity,
                 s->battery_pct, s->battery_mv,
                 s->rssi, (unsigned long)s->last_seen);

        esp_mqtt_client_publish(client, topic, payload, 0, 0, 0);
        count++;
    }

    // Publish sensor count
    snprintf(topic, sizeof(topic), "%s/hub/sensor_count", CONFIG_MQTT_TOPIC_PREFIX);
    snprintf(payload, sizeof(payload), "%d", count);
    esp_mqtt_client_publish(client, topic, payload, 0, 0, 1);

    if (count > 0) {
        ESP_LOGI(TAG, "Published readings for %d sensor(s)", count);
    }
}

bool mqtt_is_connected(void)
{
    return connected;
}

int mqtt_get_error_count(void)
{
    return error_count;
}
