#include "http_push.h"
#include "sensor_db.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_http_client.h"

static const char *TAG = "http_push";

void http_push_init(void)
{
    ESP_LOGI(TAG, "HTTP push initialized");
}

#ifdef CONFIG_HTTP_PUSH_ENABLED
void http_push_readings(void)
{
    // Build JSON array of readings
    char body[2048];
    int pos = 0;

    pos += snprintf(body + pos, sizeof(body) - pos,
                    "{\"hub_id\":\"hub-001\",\"readings\":[");

    int count = 0;
    for (int i = 0; i < CONFIG_MAX_SENSORS; i++) {
        const sensor_entry_t *s = sensor_db_get(i);
        if (!s) continue;

        if (count > 0) {
            pos += snprintf(body + pos, sizeof(body) - pos, ",");
        }

        pos += snprintf(body + pos, sizeof(body) - pos,
                        "{\"mac\":\"%s\",\"name\":\"%s\","
                        "\"temperature\":%.2f,\"humidity\":%.2f,"
                        "\"battery_pct\":%d,\"rssi\":%d,"
                        "\"timestamp\":%lu}",
                        s->mac_str, s->name,
                        s->temperature, s->humidity,
                        s->battery_pct, s->rssi,
                        (unsigned long)s->last_seen);
        count++;
    }

    pos += snprintf(body + pos, sizeof(body) - pos, "]}");

    if (count == 0) return;

    esp_http_client_config_t config = {
        .url = CONFIG_HTTP_PUSH_URL,
        .method = HTTP_METHOD_POST,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, body, strlen(body));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP push: %d sensors, status=%d", count, status);
    } else {
        ESP_LOGE(TAG, "HTTP push failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}
#else
void http_push_readings(void)
{
    // HTTP push disabled
}
#endif
