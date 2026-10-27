#pragma once

#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"

#define BUTTON_WAIT_FOREVER UINT32_MAX

/* Call once, before button_add(). */
esp_err_t button_service_init(void);

/* Expects the button wired between `pin` and GND: input, pull-up, falling edge. */
esp_err_t button_add(gpio_num_t pin);

/* Blocks until a button is pressed; returns its pin, or GPIO_NUM_NC on timeout. */
gpio_num_t button_wait(uint32_t timeout_ms);

/* Drops queued presses, so a long action is not re-triggered by them. */
void button_flush(void);
