#include "http_push.h"
#include "sensor_db.h"
#include "config_manager.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_http_client.h"

static const char *TAG = "http_push";

void http_push_init(void)
{
    ESP_LOGI(TAG, "HTTP push initialized");
}

void http_push_readings(void)
{
    const hub_config_t *cfg = config_get();
    
    // Skip if URL is empty
    if (strlen(cfg->api_url) == 0) {
        ESP_LOGD(TAG, "API URL not configured, skipping HTTP push");
        return;
    }

    // Build JSON payload with hub_id and readings array
    char body[2048];
    int pos = 0;

    pos += snprintf(body + pos, sizeof(body) - pos,
                    "{\"hub_id\":\"%s\",\"readings\":[", cfg->hub_name);

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
                        "\"battery_pct\":%d,\"battery_mv\":%d,"
                        "\"rssi\":%d,\"timestamp\":%lu}",
                        s->mac_str, s->name,
                        s->temperature, s->humidity,
                        s->battery_pct, s->battery_mv,
                        s->rssi,
                        (unsigned long)s->last_seen);
        count++;
    }

    pos += snprintf(body + pos, sizeof(body) - pos, "]}");

    if (count == 0) {
        ESP_LOGD(TAG, "No active sensors to push");
        return;
    }

    esp_http_client_config_t http_cfg = {
        .url = cfg->api_url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&http_cfg);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    
    // Add Authorization header if API key is configured
    if (strlen(cfg->api_key) > 0) {
        char auth_header[128];
        snprintf(auth_header, sizeof(auth_header), "Bearer %s", cfg->api_key);
        esp_http_client_set_header(client, "Authorization", auth_header);
    }
    
    esp_http_client_set_post_field(client, body, strlen(body));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        if (status >= 200 && status < 300) {
            ESP_LOGI(TAG, "✓ Pushed %d sensor(s) to %s (HTTP %d)", 
                     count, cfg->api_url, status);
        } else {
            ESP_LOGW(TAG, "HTTP push returned status %d", status);
        }
    } else {
        ESP_LOGE(TAG, "HTTP push failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

void http_push_health(uint64_t uptime_sec, uint32_t free_heap, 
                      int sensor_count, int wifi_rssi)
{
    const hub_config_t *cfg = config_get();
    
    // Determine health endpoint (replace /readings with /health)
    char health_url[CONFIG_URL_MAX_LEN];
    strlcpy(health_url, cfg->api_url, sizeof(health_url));
    
    // Replace last path segment with "health"
    char *last_slash = strrchr(health_url, '/');
    if (last_slash) {
        strcpy(last_slash + 1, "health");
    } else {
        ESP_LOGW(TAG, "Invalid API URL format, cannot derive health endpoint");
        return;
    }

    // Build health JSON
    char body[256];
    snprintf(body, sizeof(body),
             "{\"hub_id\":\"%s\","
             "\"uptime\":%llu,"
             "\"free_heap\":%lu,"
             "\"sensors\":%d,"
             "\"wifi_rssi\":%d}",
             cfg->hub_name,
             uptime_sec, free_heap,
             sensor_count, wifi_rssi);

    esp_http_client_config_t http_cfg = {
        .url = health_url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&http_cfg);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    
    // Add Authorization header if API key is configured
    if (strlen(cfg->api_key) > 0) {
        char auth_header[128];
        snprintf(auth_header, sizeof(auth_header), "Bearer %s", cfg->api_key);
        esp_http_client_set_header(client, "Authorization", auth_header);
    }
    
    esp_http_client_set_post_field(client, body, strlen(body));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "Health pushed (uptime=%llus, heap=%lu, sensors=%d, rssi=%d) → HTTP %d",
                 uptime_sec, free_heap, sensor_count, wifi_rssi, status);
    } else {
        ESP_LOGE(TAG, "Health push failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}
