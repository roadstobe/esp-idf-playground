#include "button.h"

#include "esp_attr.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

enum {  // BUTTON_MAX is an array size, so it must be a constant expression
    BUTTON_MAX = 4,
    PRESS_QUEUE_LEN = 8,
};

static const int64_t DEBOUNCE_US = 50 * 1000;

typedef struct {
    gpio_num_t pin;
    int64_t last_press_us;
} button_t;

static button_t buttons[BUTTON_MAX];
static size_t button_count;
static QueueHandle_t press_queue;

static void IRAM_ATTR button_isr(void *arg) {
    button_t *button = (button_t *)arg;

    // Contacts bounce for a few ms per push: keep the first edge, drop the rest.
    int64_t now_us = esp_timer_get_time();
    if (now_us - button->last_press_us < DEBOUNCE_US) {
        return;
    }
    button->last_press_us = now_us;

    BaseType_t higher_task_woken = pdFALSE;
    xQueueSendFromISR(press_queue, &button->pin, &higher_task_woken);
    portYIELD_FROM_ISR(higher_task_woken);
}

esp_err_t button_service_init(void) {
    if (press_queue != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    press_queue = xQueueCreate(PRESS_QUEUE_LEN, sizeof(gpio_num_t));
    if (press_queue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = gpio_install_isr_service(0);
    if (err == ESP_ERR_INVALID_STATE) {  // already installed by someone else
        err = ESP_OK;
    }
    return err;
}

esp_err_t button_add(gpio_num_t pin) {
    if (press_queue == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (button_count >= BUTTON_MAX) {
        return ESP_ERR_NO_MEM;
    }

    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << pin,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };

    esp_err_t err = gpio_config(&config);
    if (err != ESP_OK) {
        return err;
    }

    button_t *button = &buttons[button_count];
    *button = (button_t){ .pin = pin, .last_press_us = 0 };

    // The handler gets its own button back through `arg`.
    err = gpio_isr_handler_add(pin, button_isr, button);
    if (err != ESP_OK) {
        return err;
    }

    button_count++;
    return ESP_OK;
}

gpio_num_t button_wait(uint32_t timeout_ms) {
    const TickType_t ticks = (timeout_ms == BUTTON_WAIT_FOREVER)
        ? portMAX_DELAY
        : pdMS_TO_TICKS(timeout_ms);

    gpio_num_t pin;
    if (xQueueReceive(press_queue, &pin, ticks) != pdTRUE) {
        return GPIO_NUM_NC;
    }
    return pin;
}

void button_flush(void) {
    xQueueReset(press_queue);
}
