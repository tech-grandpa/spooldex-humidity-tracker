#pragma once

/**
 * Initialize MQTT client and connect to broker.
 */
void mqtt_client_init(void);

/**
 * Publish all current sensor readings to MQTT.
 * Called periodically by the main loop.
 */
void mqtt_publish_readings(void);

/**
 * Check if MQTT is connected.
 */
bool mqtt_is_connected(void);

/**
 * Get MQTT error count.
 */
int mqtt_get_error_count(void);
