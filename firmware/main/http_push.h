#pragma once

/**
 * Initialize HTTP push client.
 */
void http_push_init(void);

/**
 * Push all current sensor readings via HTTP POST.
 */
void http_push_readings(void);
