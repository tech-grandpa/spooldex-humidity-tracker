#pragma once

typedef enum {
    LED_MODE_OFF,
    LED_MODE_FAST_BLINK,    // AP config mode
    LED_MODE_SLOW_BLINK,    // Connecting to WiFi
    LED_MODE_SOLID,         // Connected, scanning
    LED_MODE_DOUBLE_BLINK,  // Sensor data received
} led_mode_t;

/**
 * Initialize status LED.
 */
void status_led_init(void);

/**
 * Set LED blink mode.
 */
void status_led_set_mode(led_mode_t mode);

/**
 * Trigger a double-blink (used when sensor data arrives).
 */
void status_led_pulse(void);
