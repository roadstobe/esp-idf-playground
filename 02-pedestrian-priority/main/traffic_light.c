#include "traffic_light.h"

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct {
    bool red;
    bool yellow;
    bool green;
} lamps_t;

static const lamps_t STATE_LAMPS[] = {
    [TRAFFIC_LIGHT_OFF]        = { false, false, false },
    [TRAFFIC_LIGHT_RED]        = { true,  false, false },
    [TRAFFIC_LIGHT_RED_YELLOW] = { true,  true,  false },
    [TRAFFIC_LIGHT_YELLOW]     = { false, true,  false },
    [TRAFFIC_LIGHT_GREEN]      = { false, false, true  },
};

_Static_assert(sizeof(STATE_LAMPS) / sizeof(STATE_LAMPS[0]) == TRAFFIC_LIGHT_STATE_COUNT,
               "every traffic_light_state_t needs a row in STATE_LAMPS");

esp_err_t traffic_light_init(traffic_light_t *light) {
    const gpio_config_t config = {
        .pin_bit_mask = (1ULL << light->red_pin)
                      | (1ULL << light->yellow_pin)
                      | (1ULL << light->green_pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&config);
    if (err != ESP_OK) {
        return err;
    }

    traffic_light_set(light, TRAFFIC_LIGHT_OFF);
    return ESP_OK;
}

void traffic_light_set(traffic_light_t *light, traffic_light_state_t state) {
    const lamps_t lamps = STATE_LAMPS[state];

    gpio_set_level(light->red_pin, lamps.red);
    gpio_set_level(light->yellow_pin, lamps.yellow);
    gpio_set_level(light->green_pin, lamps.green);
}

void traffic_light_blink(traffic_light_t *light, traffic_light_state_t state,
                         uint8_t times, uint32_t interval_ms) {
    const TickType_t interval = pdMS_TO_TICKS(interval_ms);

    for (uint8_t i = 0; i < times; i++) {
        traffic_light_set(light, state);
        vTaskDelay(interval);
        traffic_light_set(light, TRAFFIC_LIGHT_OFF);
        vTaskDelay(interval);
    }
}
