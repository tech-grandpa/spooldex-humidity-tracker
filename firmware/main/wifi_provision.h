#pragma once

#include <stdbool.h>

/**
 * Start WiFi provisioning in AP mode with captive portal.
 * Runs a simple web server for configuring WiFi and MQTT.
 * Blocks until configuration is saved, then reboots.
 */
void wifi_provision_start(void);

/**
 * Check if we should enter provisioning mode.
 * Returns true if no WiFi credentials are configured.
 */
bool wifi_provision_needed(void);
