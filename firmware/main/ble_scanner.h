#pragma once

#include <stdint.h>

/**
 * Parsed sensor reading from a pvvx BLE advertisement.
 */
typedef struct {
    uint8_t  mac[6];
    char     name[20];
    float    temperature;    // °C
    float    humidity;       // % RH
    uint8_t  battery_pct;
    uint16_t battery_mv;
    int8_t   rssi;
} pvvx_reading_t;

/**
 * Initialize BLE scanner (NimBLE).
 * Starts passive scanning for pvvx-format advertisements.
 */
void ble_scanner_init(void);

/**
 * Start a scan cycle.
 */
void ble_scanner_start(void);

/**
 * Stop scanning.
 */
void ble_scanner_stop(void);
