#include "buzzer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#define BUZZER_GPIO ((gpio_num_t)CONFIG_BUZZER_GPIO)

#ifdef CONFIG_BUZZER_ACTIVE_LOW
#define LEVEL_ON  0
#define LEVEL_OFF 1
#else
#define LEVEL_ON  1
#define LEVEL_OFF 0
#endif

#define ALARM_ON_MS  400
#define ALARM_OFF_MS 400
#define CHIRP_ON_MS  120
#define CHIRP_OFF_MS 120

static const char *TAG = "BUZZER";

static TaskHandle_t buzzer_task_handle = NULL;
static volatile bool alarm_active = false;
static volatile uint8_t chirps_pending = 0;

static void buzzer_task(void *pvParameters)
{
    (void)pvParameters;

    while (1)
    {
        if (alarm_active)
        {
            gpio_set_level(BUZZER_GPIO, LEVEL_ON);
            vTaskDelay(pdMS_TO_TICKS(ALARM_ON_MS));
            gpio_set_level(BUZZER_GPIO, LEVEL_OFF);
            vTaskDelay(pdMS_TO_TICKS(ALARM_OFF_MS));
        }
        else if (chirps_pending > 0)
        {
            chirps_pending--;
            gpio_set_level(BUZZER_GPIO, LEVEL_ON);
            vTaskDelay(pdMS_TO_TICKS(CHIRP_ON_MS));
            gpio_set_level(BUZZER_GPIO, LEVEL_OFF);
            vTaskDelay(pdMS_TO_TICKS(CHIRP_OFF_MS));
        }
        else
        {
            gpio_set_level(BUZZER_GPIO, LEVEL_OFF);
            // Ceka dok neko ne zatrazi zvuk.
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }
    }
}

esp_err_t buzzer_init(void)
{
    gpio_config_t config = {
        .pin_bit_mask = 1ULL << BUZZER_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t err = gpio_config(&config);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Greska pri konfiguraciji GPIO%d: %s",
                 BUZZER_GPIO, esp_err_to_name(err));
        return err;
    }

    err = gpio_set_level(BUZZER_GPIO, LEVEL_OFF);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Greska pri iskljucivanju buzzera: %s",
                 esp_err_to_name(err));
        return err;
    }

    if (xTaskCreate(buzzer_task, "buzzer_task", 2048, NULL, 6,
                    &buzzer_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Ne mogu da kreiram buzzer_task");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Buzzer inicijalizovan na GPIO%d", BUZZER_GPIO);

    return ESP_OK;
}

esp_err_t buzzer_on(void)
{
    return gpio_set_level(BUZZER_GPIO, LEVEL_ON);
}

esp_err_t buzzer_off(void)
{
    return gpio_set_level(BUZZER_GPIO, LEVEL_OFF);
}

void buzzer_set_alarm(bool active)
{
    if (alarm_active == active) {
        return;
    }

    alarm_active = active;
    ESP_LOGI(TAG, "Alarm zujalice: %s", active ? "UKLJUCEN" : "iskljucen");

    if (buzzer_task_handle != NULL) {
        xTaskNotifyGive(buzzer_task_handle);
    }
}

bool buzzer_alarm_active(void)
{
    return alarm_active;
}

void buzzer_chirp(uint8_t count)
{
    chirps_pending = count;

    if (buzzer_task_handle != NULL) {
        xTaskNotifyGive(buzzer_task_handle);
    }
}
