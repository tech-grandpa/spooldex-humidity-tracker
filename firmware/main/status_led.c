#include "status_led.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"

static const char *TAG = "status_led";

#ifndef CONFIG_STATUS_LED_PIN
#define CONFIG_STATUS_LED_PIN 8  // Default GPIO for ESP32-C6
#endif

static led_mode_t current_mode = LED_MODE_OFF;
static TimerHandle_t led_timer = NULL;
static int pulse_count = 0;

static void led_on(void)
{
    gpio_set_level(CONFIG_STATUS_LED_PIN, 1);
}

static void led_off(void)
{
    gpio_set_level(CONFIG_STATUS_LED_PIN, 0);
}

static void led_timer_callback(TimerHandle_t timer)
{
    static bool state = false;

    if (pulse_count > 0) {
        // Double blink sequence
        state = !state;
        gpio_set_level(CONFIG_STATUS_LED_PIN, state);
        pulse_count--;
        if (pulse_count == 0) {
            // Resume previous mode after pulse
            xTimerStop(timer, 0);
            status_led_set_mode(current_mode);
        }
        return;
    }

    switch (current_mode) {
    case LED_MODE_FAST_BLINK:
        state = !state;
        gpio_set_level(CONFIG_STATUS_LED_PIN, state);
        xTimerChangePeriod(timer, pdMS_TO_TICKS(200), 0);
        break;

    case LED_MODE_SLOW_BLINK:
        state = !state;
        gpio_set_level(CONFIG_STATUS_LED_PIN, state);
        xTimerChangePeriod(timer, pdMS_TO_TICKS(1000), 0);
        break;

    case LED_MODE_SOLID:
        led_on();
        xTimerStop(timer, 0);
        break;

    case LED_MODE_OFF:
    default:
        led_off();
        xTimerStop(timer, 0);
        break;
    }
}

void status_led_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_STATUS_LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    led_off();

    led_timer = xTimerCreate("led_timer", pdMS_TO_TICKS(200), pdTRUE, NULL, led_timer_callback);

    ESP_LOGI(TAG, "Status LED initialized on GPIO %d", CONFIG_STATUS_LED_PIN);
}

void status_led_set_mode(led_mode_t mode)
{
    if (pulse_count > 0) return;  // Don't interrupt pulse

    current_mode = mode;

    if (mode == LED_MODE_OFF) {
        xTimerStop(led_timer, 0);
        led_off();
    } else if (mode == LED_MODE_SOLID) {
        xTimerStop(led_timer, 0);
        led_on();
    } else {
        xTimerStart(led_timer, 0);
    }
}

void status_led_pulse(void)
{
    pulse_count = 4;  // Two on/off cycles = double blink
    xTimerChangePeriod(led_timer, pdMS_TO_TICKS(100), 0);
    xTimerStart(led_timer, 0);
}
