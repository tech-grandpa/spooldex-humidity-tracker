#include "ble_scanner.h"
#include "sensor_db.h"
#include "status_led.h"
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

/*
 * BTHome v2 format:
 * UUID16 service data: 0xFCD2
 * Byte 0: Device info (bit 0 = encryption, bits 1-4 = version, bit 5 = trigger)
 * Bytes 1+: Object ID + data pairs
 *
 * Common object IDs:
 * 0x01 = Battery (1 byte, %)
 * 0x02 = Temperature (2 bytes, 0.01°C)
 * 0x03 = Humidity (2 bytes, 0.01%)
 * 0x0C = Voltage (2 bytes, 0.001V)
 */
#define BTHOME_SERVICE_UUID 0xFCD2

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
 * Parse BTHome v2 format advertisement.
 * Returns true if successfully parsed.
 */
static bool parse_bthome_adv(const uint8_t *data, uint8_t len, const uint8_t *mac_addr, pvvx_reading_t *out)
{
    if (len < 2) return false;

    // Byte 0: device info
    uint8_t device_info = data[0];
    bool encrypted = device_info & 0x01;
    
    if (encrypted) {
        ESP_LOGD(TAG, "BTHome: encrypted advertisements not supported");
        return false;
    }

    // Copy MAC address
    memcpy(out->mac, mac_addr, 6);

    // Parse object ID + data pairs
    uint8_t pos = 1;
    bool has_temp = false, has_humi = false;
    
    while (pos < len) {
        uint8_t obj_id = data[pos++];
        
        switch (obj_id) {
        case 0x01:  // Battery %
            if (pos < len) {
                out->battery_pct = data[pos++];
            }
            break;
            
        case 0x02:  // Temperature (0.01°C)
            if (pos + 1 < len) {
                int16_t temp_raw = (int16_t)(data[pos] | (data[pos + 1] << 8));
                out->temperature = temp_raw / 100.0f;
                pos += 2;
                has_temp = true;
            }
            break;
            
        case 0x03:  // Humidity (0.01%)
            if (pos + 1 < len) {
                uint16_t humi_raw = (uint16_t)(data[pos] | (data[pos + 1] << 8));
                out->humidity = humi_raw / 100.0f;
                pos += 2;
                has_humi = true;
            }
            break;
            
        case 0x0C:  // Voltage (0.001V)
            if (pos + 1 < len) {
                uint16_t volt_raw = (uint16_t)(data[pos] | (data[pos + 1] << 8));
                out->battery_mv = volt_raw;  // Already in mV
                pos += 2;
            }
            break;
            
        default:
            // Unknown object ID — try to skip it
            // Most BTHome objects are 1-3 bytes, assume 1 for unknown
            if (pos < len) pos++;
            break;
        }
    }

    return has_temp && has_humi;
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

    // Look for service data (pvvx or BTHome)
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
            const uint8_t *payload = &ad[pos + 4];
            uint8_t payload_len = elem_len - 3;  // minus type + uuid bytes

            pvvx_reading_t reading = {0};
            bool parsed = false;

            if (uuid == PVVX_SERVICE_UUID) {
                parsed = parse_pvvx_adv(payload, payload_len, &reading);
            } else if (uuid == BTHOME_SERVICE_UUID) {
                parsed = parse_bthome_adv(payload, payload_len, desc->addr.val, &reading);
            }

            if (parsed) {
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

                // Trigger LED pulse to indicate sensor data received
                status_led_pulse();
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
