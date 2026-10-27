#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "traffic_light";

static const uint32_t RED_MS          = 10000;
static const uint32_t YELLOW_MS       = 2000;
static const uint32_t GREEN_MS        = 15000;
static const uint32_t GREEN_BLINK_MS  = 500;
static const uint32_t GREEN_PAUSE_MS  = 500;
static const uint8_t  GREEN_BLINKS    = 3;

typedef enum {
    LED_RED,
    LED_YELLOW,
    LED_GREEN,
    LED_COUNT,
} led_t;

typedef enum {
    TRAFFIC_RED,
    TRAFFIC_RED_YELLOW,
    TRAFFIC_GREEN,
    TRAFFIC_GREEN_BLINK,
    TRAFFIC_YELLOW,
} traffic_state_t;

static const gpio_num_t LED_PINS[LED_COUNT] = {
    [LED_RED]    = GPIO_NUM_14,
    [LED_YELLOW] = GPIO_NUM_13,
    [LED_GREEN]  = GPIO_NUM_12,
};

static void set_lights(bool red, bool yellow, bool green) {
    gpio_set_level(LED_PINS[LED_RED], red);
    gpio_set_level(LED_PINS[LED_YELLOW], yellow);
    gpio_set_level(LED_PINS[LED_GREEN], green);
}

static void leds_init(void) {
    uint64_t pin_mask = 0;
    for (size_t i = 0; i < LED_COUNT; i++) {
        pin_mask |= 1ULL << LED_PINS[i];
    }

    const gpio_config_t config = {
        .pin_bit_mask = pin_mask,
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&config));
    set_lights(false, false, false);
}

static void blink_green(void) {
    for (uint8_t i = 0; i < GREEN_BLINKS; i++) {
        set_lights(false, false, true);
        vTaskDelay(pdMS_TO_TICKS(GREEN_BLINK_MS));
        set_lights(false, false, false);
        vTaskDelay(pdMS_TO_TICKS(GREEN_BLINK_MS));
    }
}

/* Lights up `state`, waits out its phase and returns the state that follows. */
static traffic_state_t traffic_light_step(traffic_state_t state) {
    switch (state) {
        case TRAFFIC_RED:
            ESP_LOGI(TAG, "RED");
            set_lights(true, false, false);
            vTaskDelay(pdMS_TO_TICKS(RED_MS));
            return TRAFFIC_RED_YELLOW;

        case TRAFFIC_RED_YELLOW:
            ESP_LOGI(TAG, "RED + YELLOW");
            set_lights(true, true, false);
            vTaskDelay(pdMS_TO_TICKS(YELLOW_MS));
            return TRAFFIC_GREEN;

        case TRAFFIC_GREEN:
            ESP_LOGI(TAG, "GREEN");
            set_lights(false, false, true);
            vTaskDelay(pdMS_TO_TICKS(GREEN_MS));

            set_lights(false, false, false);
            vTaskDelay(pdMS_TO_TICKS(GREEN_PAUSE_MS));
            return TRAFFIC_GREEN_BLINK;

        case TRAFFIC_GREEN_BLINK:
            ESP_LOGI(TAG, "GREEN BLINK");
            blink_green();
            return TRAFFIC_YELLOW;

        case TRAFFIC_YELLOW:
            ESP_LOGI(TAG, "YELLOW");
            set_lights(false, true, false);
            vTaskDelay(pdMS_TO_TICKS(YELLOW_MS));
            return TRAFFIC_RED;
    }

    // No default branch: -Wswitch then warns about a state added but not handled.
    return TRAFFIC_RED;
}

void app_main(void) {
    leds_init();

    traffic_state_t state = TRAFFIC_RED;
    while (true) {
        state = traffic_light_step(state);
    }
}
