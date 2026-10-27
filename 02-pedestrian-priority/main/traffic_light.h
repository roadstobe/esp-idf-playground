#pragma once

#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"

typedef struct {
    gpio_num_t red_pin;
    gpio_num_t yellow_pin;
    gpio_num_t green_pin;
} traffic_light_t;

typedef enum {
    TRAFFIC_LIGHT_OFF,
    TRAFFIC_LIGHT_RED,
    TRAFFIC_LIGHT_RED_YELLOW,
    TRAFFIC_LIGHT_YELLOW,
    TRAFFIC_LIGHT_GREEN,
    TRAFFIC_LIGHT_STATE_COUNT,  // keep last
} traffic_light_state_t;

esp_err_t traffic_light_init(traffic_light_t *light);

void traffic_light_set(traffic_light_t *light, traffic_light_state_t state);

/* Blocks the calling task and ends with every lamp off. */
void traffic_light_blink(traffic_light_t *light, traffic_light_state_t state,
                         uint8_t times, uint32_t interval_ms);
