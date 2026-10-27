#include <inttypes.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "button.h"
#include "traffic_light.h"

static const char *TAG = "crossing";

static const gpio_num_t BUTTON_PIN = GPIO_NUM_7;
static const gpio_num_t RED_PIN    = GPIO_NUM_14;
static const gpio_num_t YELLOW_PIN = GPIO_NUM_13;
static const gpio_num_t GREEN_PIN  = GPIO_NUM_12;

/* Cars always get MIN of green, and after a request always GRACE of warning. */
static const int64_t CAR_GREEN_MIN_MS   = 15000;
static const int64_t CAR_GREEN_GRACE_MS = 5000;

static const uint8_t  GREEN_BLINK_TIMES       = 3;
static const uint32_t GREEN_BLINK_INTERVAL_MS = 300;
static const uint32_t YELLOW_MS               = 2000;
static const uint32_t WALK_MS                 = 10000;
static const uint32_t RED_YELLOW_MS           = 2000;

static int64_t now_ms(void) {
    return esp_timer_get_time() / 1000;
}

static uint32_t green_hold_ms(int64_t green_elapsed_ms) {
    // Signed: the minimum is usually spent already and this goes negative.
    const int64_t left_of_minimum = CAR_GREEN_MIN_MS - green_elapsed_ms;

    return (uint32_t)(left_of_minimum > CAR_GREEN_GRACE_MS ? left_of_minimum
                                                           : CAR_GREEN_GRACE_MS);
}

static void run_crossing(traffic_light_t *light) {
    traffic_light_blink(light, TRAFFIC_LIGHT_GREEN, GREEN_BLINK_TIMES, GREEN_BLINK_INTERVAL_MS);

    traffic_light_set(light, TRAFFIC_LIGHT_YELLOW);
    vTaskDelay(pdMS_TO_TICKS(YELLOW_MS));

    ESP_LOGI(TAG, "walk");
    traffic_light_set(light, TRAFFIC_LIGHT_RED);
    vTaskDelay(pdMS_TO_TICKS(WALK_MS));

    traffic_light_set(light, TRAFFIC_LIGHT_RED_YELLOW);
    vTaskDelay(pdMS_TO_TICKS(RED_YELLOW_MS));

    traffic_light_set(light, TRAFFIC_LIGHT_GREEN);
}

void app_main(void) {
    traffic_light_t light = {
        .red_pin = RED_PIN,
        .yellow_pin = YELLOW_PIN,
        .green_pin = GREEN_PIN,
    };

    ESP_ERROR_CHECK(traffic_light_init(&light));
    ESP_ERROR_CHECK(button_service_init());
    ESP_ERROR_CHECK(button_add(BUTTON_PIN));

    traffic_light_set(&light, TRAFFIC_LIGHT_GREEN);
    int64_t green_since_ms = now_ms();

    while (true) {
        if (button_wait(BUTTON_WAIT_FOREVER) != BUTTON_PIN) {
            continue;
        }

        uint32_t hold_ms = green_hold_ms(now_ms() - green_since_ms);
        ESP_LOGI(TAG, "request accepted, green holds for %" PRIu32 " ms", hold_ms);
        vTaskDelay(pdMS_TO_TICKS(hold_ms));

        run_crossing(&light);
        green_since_ms = now_ms();
        button_flush();
    }
}
