#pragma once

#include <stdint.h>

/**
 * Initialize HTTP push client.
 */
void http_push_init(void);

/**
 * Push all current sensor readings via HTTP POST.
 */
void http_push_readings(void);

/**
 * Push hub health metrics via HTTP POST to /api/humidity/health.
 */
void http_push_health(uint64_t uptime_sec, uint32_t free_heap, 
                      int sensor_count, int wifi_rssi);
