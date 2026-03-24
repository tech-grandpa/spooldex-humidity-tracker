#include "wifi_provision.h"
#include "config_manager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include <string.h>

static const char *TAG = "provision";

#define AP_SSID "Spooldex-Hub-Setup"
#define AP_PASSWORD ""
#define AP_MAX_CONN 1

static httpd_handle_t server = NULL;

/* HTML form for configuration */
static const char *config_html = 
"<!DOCTYPE html><html><head><meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
"<title>Spooldex Hub Setup</title>"
"<style>"
"body{font-family:Arial,sans-serif;max-width:500px;margin:50px auto;padding:20px;background:#f0f0f0;}"
"h1{color:#333;text-align:center;}"
"form{background:white;padding:30px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}"
"label{display:block;margin:15px 0 5px;font-weight:bold;}"
"input{width:100%;padding:8px;margin-bottom:10px;border:1px solid #ddd;border-radius:4px;box-sizing:border-box;}"
"button{background:#007bff;color:white;padding:12px;width:100%;border:none;border-radius:4px;font-size:16px;cursor:pointer;margin-top:10px;}"
"button:hover{background:#0056b3;}"
".info{background:#e7f3ff;padding:10px;border-radius:4px;margin-bottom:20px;font-size:14px;}"
"</style></head><body>"
"<h1>🌡️ Spooldex Hub Setup</h1>"
"<div class='info'>Configure your hub's WiFi and API connection. The device will reboot after saving.</div>"
"<form action='/save' method='POST'>"
"<label>WiFi SSID:</label>"
"<input type='text' name='ssid' required maxlength='31' placeholder='YourWiFiNetwork'>"
"<label>WiFi Password:</label>"
"<input type='password' name='pass' required maxlength='63' placeholder='YourWiFiPassword'>"
"<label>API URL:</label>"
"<input type='text' name='api_url' required maxlength='127' placeholder='http://localhost:3000/api/humidity/readings'>"
"<label>API Key (optional):</label>"
"<input type='password' name='api_key' maxlength='63' placeholder='your-api-key'>"
"<label>Hub Name:</label>"
"<input type='text' name='name' maxlength='31' placeholder='spooldex-hub' value='spooldex-hub'>"
"<label>OTA Update URL (optional):</label>"
"<input type='text' name='ota' maxlength='127' placeholder='https://example.com/firmware.bin'>"
"<button type='submit'>Save & Reboot</button>"
"</form></body></html>";

static const char *success_html = 
"<!DOCTYPE html><html><head><meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
"<title>Saved</title>"
"<style>body{font-family:Arial,sans-serif;max-width:500px;margin:50px auto;padding:20px;text-align:center;}"
"h1{color:#28a745;}p{font-size:18px;}</style></head><body>"
"<h1>✓ Configuration Saved</h1>"
"<p>Hub is rebooting and will connect to your WiFi...</p>"
"</body></html>";

/* URL decode helper */
static void url_decode(char *dst, const char *src, size_t dst_size)
{
    size_t i = 0;
    while (*src && i < dst_size - 1) {
        if (*src == '%' && src[1] && src[2]) {
            int val;
            sscanf(src + 1, "%2x", &val);
            dst[i++] = (char)val;
            src += 3;
        } else if (*src == '+') {
            dst[i++] = ' ';
            src++;
        } else {
            dst[i++] = *src++;
        }
    }
    dst[i] = '\0';
}

/* Parse form data */
static void parse_form_data(const char *body, hub_config_t *cfg)
{
    char key[32], value[128];
    const char *p = body;

    while (*p) {
        // Extract key
        const char *eq = strchr(p, '=');
        if (!eq) break;
        size_t key_len = eq - p;
        if (key_len >= sizeof(key)) key_len = sizeof(key) - 1;
        memcpy(key, p, key_len);
        key[key_len] = '\0';

        // Extract value
        p = eq + 1;
        const char *amp = strchr(p, '&');
        size_t val_len = amp ? (amp - p) : strlen(p);
        if (val_len >= sizeof(value)) val_len = sizeof(value) - 1;
        memcpy(value, p, val_len);
        value[val_len] = '\0';
        
        // Decode and store
        if (strcmp(key, "ssid") == 0) {
            url_decode(cfg->wifi_ssid, value, sizeof(cfg->wifi_ssid));
        } else if (strcmp(key, "pass") == 0) {
            url_decode(cfg->wifi_password, value, sizeof(cfg->wifi_password));
        } else if (strcmp(key, "api_url") == 0) {
            url_decode(cfg->api_url, value, sizeof(cfg->api_url));
        } else if (strcmp(key, "api_key") == 0) {
            url_decode(cfg->api_key, value, sizeof(cfg->api_key));
        } else if (strcmp(key, "name") == 0) {
            url_decode(cfg->hub_name, value, sizeof(cfg->hub_name));
        } else if (strcmp(key, "ota") == 0) {
            url_decode(cfg->ota_url, value, sizeof(cfg->ota_url));
        }

        if (!amp) break;
        p = amp + 1;
    }
}

/* Root page handler */
static esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, config_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Save config handler */
static esp_err_t save_handler(httpd_req_t *req)
{
    char body[512];
    int ret = httpd_req_recv(req, body, sizeof(body) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    body[ret] = '\0';

    hub_config_t new_cfg = {0};
    parse_form_data(body, &new_cfg);

    ESP_LOGI(TAG, "Received config: SSID=%s, API=%s, Name=%s",
             new_cfg.wifi_ssid, new_cfg.api_url, new_cfg.hub_name);

    // Validate
    if (strlen(new_cfg.wifi_ssid) == 0 || strlen(new_cfg.api_url) == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing required fields");
        return ESP_FAIL;
    }

    // Save to NVS
    if (config_save(&new_cfg) != ESP_OK) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Send success page
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, success_html, HTTPD_RESP_USE_STRLEN);

    // Schedule reboot
    ESP_LOGI(TAG, "Configuration saved, rebooting in 2 seconds...");
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();

    return ESP_OK;
}

void wifi_provision_start(void)
{
    ESP_LOGI(TAG, "Starting WiFi provisioning AP mode");

    // Initialize WiFi in AP mode
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t ap_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .password = AP_PASSWORD,
            .max_connection = AP_MAX_CONN,
            .authmode = WIFI_AUTH_OPEN,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "AP started: SSID=%s, IP=192.168.4.1", AP_SSID);

    // Start web server
    httpd_config_t server_config = HTTPD_DEFAULT_CONFIG();
    server_config.lru_purge_enable = true;

    if (httpd_start(&server, &server_config) == ESP_OK) {
        httpd_uri_t root = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_handler,
        };
        httpd_register_uri_handler(server, &root);

        httpd_uri_t save = {
            .uri = "/save",
            .method = HTTP_POST,
            .handler = save_handler,
        };
        httpd_register_uri_handler(server, &save);

        ESP_LOGI(TAG, "Web server started at http://192.168.4.1");
        ESP_LOGI(TAG, "Connect to '%s' and open browser to configure", AP_SSID);
    } else {
        ESP_LOGE(TAG, "Failed to start web server");
    }

    // Block here — server runs until config is saved and device reboots
}

bool wifi_provision_needed(void)
{
    return !config_is_provisioned();
}
