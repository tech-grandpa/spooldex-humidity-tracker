#pragma once

#include <stdint.h>
#include <stdbool.h>

#define SENSOR_NAME_MAX_LEN 20
#define SENSOR_MAC_STR_LEN  18  // "AA:BB:CC:DD:EE:FF\0"

#ifndef CONFIG_MAX_SENSORS
#define CONFIG_MAX_SENSORS 16
#endif

typedef struct {
    uint8_t  mac[6];
    char     mac_str[SENSOR_MAC_STR_LEN];
    char     name[SENSOR_NAME_MAX_LEN];
    float    temperature;    // °C
    float    humidity;       // % RH
    uint8_t  battery_pct;   // 0-100
    uint16_t battery_mv;    // millivolts
    int8_t   rssi;          // dBm
    uint32_t last_seen;     // unix timestamp
    bool     active;
} sensor_entry_t;

/**
 * Initialize the sensor database.
 */
void sensor_db_init(void);

/**
 * Update or insert a sensor reading.
 * Returns the sensor index, or -1 if the database is full.
 */
int sensor_db_update(const uint8_t mac[6], const char *name,
                     float temperature, float humidity,
                     uint8_t battery_pct, uint16_t battery_mv,
                     int8_t rssi);

/**
 * Get a sensor entry by index.
 * Returns NULL if index is out of range or sensor is inactive.
 */
const sensor_entry_t *sensor_db_get(int index);

/**
 * Get the number of active sensors.
 */
int sensor_db_count(void);

/**
 * Mark sensors as inactive if not seen for `timeout_s` seconds.
 */
void sensor_db_prune(uint32_t timeout_s);
