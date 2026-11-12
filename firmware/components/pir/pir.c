#include "pir.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#define PIR_GPIO ((gpio_num_t)CONFIG_PIR_GPIO)

static const char *TAG = "PIR";

static QueueHandle_t pir_queue = NULL;

static void IRAM_ATTR pir_isr_handler(void *arg)
{
    (void)arg;

    pir_edge_t edge = {
        .motion = gpio_get_level(PIR_GPIO) == 1,
        .tick = xTaskGetTickCountFromISR(),
    };

    BaseType_t higher_prio_woken = pdFALSE;
    xQueueSendFromISR(pir_queue, &edge, &higher_prio_woken);

    if (higher_prio_woken) {
        portYIELD_FROM_ISR();
    }
}

esp_err_t pir_init(QueueHandle_t edge_queue)
{
    if (edge_queue == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    pir_queue = edge_queue;

    gpio_config_t config = {
        .pin_bit_mask = 1ULL << PIR_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_ANYEDGE};

    esp_err_t err = gpio_config(&config);

    if (err != ESP_OK) {
        return err;
    }

    err = gpio_install_isr_service(0);

    // Servis je mozda vec instaliran od druge komponente.
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    err = gpio_isr_handler_add(PIR_GPIO, pir_isr_handler, NULL);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "PIR senzor na GPIO%d (prekid na obe ivice)", PIR_GPIO);
    }

    return err;
}

bool pir_motion_detected(void)
{
    return gpio_get_level(PIR_GPIO) == 1;
}
