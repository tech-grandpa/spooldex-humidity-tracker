#pragma once

/**
 * Initialize the SSD1306 OLED display.
 */
void display_init(void);

/**
 * Update the display with current sensor readings.
 * Cycles through sensors if more than can fit on screen.
 */
void display_update(void);
