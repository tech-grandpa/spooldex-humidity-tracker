#include "ble_scanner.h"
#include "sensor_db.h"
#include "esp_log.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

static const char *TAG = "ble_scanner";

/*
 * pvvx custom format advertisement payload (17 bytes):
 *
 * Offset  Size  Description
 * 0       6     MAC address (little-endian)
 * 6       2     Temperature (int16_t, 0.01°C)
 * 8       2     Humidity (uint16_t, 0.01%)
 * 10      2     Battery voltage (uint16_t, mV)
 * 12      1     Battery level (uint8_t, %)
 * 13      1     Counter
 * 14      1     Flags
 *
 * Identified by UUID16 service data: 0x181A (Environmental Sensing)
 */
#define PVVX_SERVICE_UUID   0x181A
#define PVVX_PAYLOAD_LEN    15

/**
 * Parse a pvvx custom format advertisement.
 * Returns true if successfully parsed.
 */
static bool parse_pvvx_adv(const uint8_t *data, uint8_t len, pvvx_reading_t *out)
{
    if (len < PVVX_PAYLOAD_LEN) {
        return false;
    }

    // MAC (bytes 0-5, little-endian — reverse for standard notation)
    for (int i = 0; i < 6; i++) {
        out->mac[i] = data[5 - i];
    }

    // Temperature: int16_t at offset 6, in 0.01°C
    int16_t temp_raw = (int16_t)(data[6] | (data[7] << 8));
    out->temperature = temp_raw / 100.0f;

    // Humidity: uint16_t at offset 8, in 0.01%
    uint16_t humi_raw = (uint16_t)(data[8] | (data[9] << 8));
    out->humidity = humi_raw / 100.0f;

    // Battery voltage: uint16_t at offset 10, in mV
    out->battery_mv = (uint16_t)(data[10] | (data[11] << 8));

    // Battery percentage: uint8_t at offset 12
    out->battery_pct = data[12];

    return true;
}

/**
 * BLE GAP event handler — processes incoming advertisements.
 */
static int ble_gap_event_handler(struct ble_gap_event *event, void *arg)
{
    if (event->type != BLE_GAP_EVENT_DISC) {
        return 0;
    }

    struct ble_gap_disc_desc *desc = &event->disc;

    // Walk advertisement data looking for UUID16 service data (type 0x16)
    struct ble_hs_adv_fields fields;
    if (ble_hs_adv_parse_fields(&fields, desc->data, desc->length_data) != 0) {
        return 0;
    }

    // Look for service data with Environmental Sensing UUID (0x181A)
    // We need to manually walk the raw AD structures for service data
    const uint8_t *ad = desc->data;
    uint8_t ad_len = desc->length_data;
    uint8_t pos = 0;

    while (pos < ad_len) {
        uint8_t elem_len = ad[pos];
        if (elem_len == 0 || pos + elem_len >= ad_len) break;

        uint8_t ad_type = ad[pos + 1];

        // AD type 0x16 = Service Data - 16-bit UUID
        if (ad_type == 0x16 && elem_len >= 4) {
            uint16_t uuid = ad[pos + 2] | (ad[pos + 3] << 8);

            if (uuid == PVVX_SERVICE_UUID) {
                const uint8_t *payload = &ad[pos + 4];
                uint8_t payload_len = elem_len - 3;  // minus type + uuid bytes

                pvvx_reading_t reading = {0};
                if (parse_pvvx_adv(payload, payload_len, &reading)) {
                    reading.rssi = desc->rssi;

                    // Extract name from advertisement if available
                    if (fields.name_len > 0 && fields.name_len < sizeof(reading.name)) {
                        memcpy(reading.name, fields.name, fields.name_len);
                        reading.name[fields.name_len] = '\0';
                    }

                    // Store in sensor database
                    sensor_db_update(
                        reading.mac, reading.name,
                        reading.temperature, reading.humidity,
                        reading.battery_pct, reading.battery_mv,
                        reading.rssi
                    );
                }
                break;
            }
        }
        pos += elem_len + 1;
    }

    return 0;
}

static void ble_host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void ble_scanner_init(void)
{
    ESP_LOGI(TAG, "Initializing BLE scanner");

    nimble_port_init();
    nimble_port_freertos_init(ble_host_task);

    ESP_LOGI(TAG, "BLE scanner ready");
}

void ble_scanner_start(void)
{
    struct ble_gap_disc_params scan_params = {
        .itvl = 0,    // use default
        .window = 0,  // use default
        .filter_policy = BLE_HCI_SCAN_FILT_NO_WL,
        .limited = 0,
        .passive = 1,  // passive scan — no scan requests sent
        .filter_duplicates = 0,  // we want every advertisement
    };

    int rc = ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER,
                          &scan_params, ble_gap_event_handler, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to start BLE scan: %d", rc);
    } else {
        ESP_LOGI(TAG, "BLE scanning started (passive)");
    }
}

void ble_scanner_stop(void)
{
    ble_gap_disc_cancel();
    ESP_LOGI(TAG, "BLE scanning stopped");
}
