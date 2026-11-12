#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_err.h"

#include "nvs.h"
#include "dht11.h"
#include "wifi_manager.h"
#include "http_client.h"
#include "services.h"

static const char *TAG = "MAIN";

typedef struct
{
    float temperature;
    float humidity;
} sensor_data_t;

static QueueHandle_t sensor_queue = NULL;

static void sensor_task(void *pvParameters)
{
    float temperature;
    float humidity;

    TickType_t last_wake_time = xTaskGetTickCount();

    while (1)
    {
        esp_err_t err = dht11_read(&temperature, &humidity);

        if (err == ESP_OK)
        {
            ESP_LOGI(TAG,
                     "Temperatura: %.1f C | Vlaznost: %.1f %%",
                     (double)temperature,
                     (double)humidity);

            sensor_data_t data = {
                .temperature = temperature,
                .humidity = humidity};

            xQueueOverwrite(sensor_queue, &data);
        }
        else
        {
            ESP_LOGE(TAG,
                     "Greska pri citanju DHT11 senzora: %s",
                     esp_err_to_name(err));
        }

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(5000));
    }
}

static void http_task(void *pvParameters)
{
    sensor_data_t received_data;
    while (1)
    {
        if (xQueueReceive(sensor_queue,
                          &received_data,
                          portMAX_DELAY) == pdTRUE)
        {
            if (wifi_manager_is_connected())
            {
                ESP_LOGI(TAG, "Wi-Fi status: povezan");
                esp_err_t err = send_temperature_and_humidity(
                    received_data.temperature,
                    received_data.humidity);
                if (err == ESP_OK)
                {
                    ESP_LOGI(TAG, "Podaci su uspesno poslati");
                }
                else
                {
                    ESP_LOGE(TAG,
                             "Slanje podataka nije uspelo: %s",
                             esp_err_to_name(err));
                }
            }
            else
            {
                ESP_LOGW(TAG,
                         "Wi-Fi nije povezan. Podaci nisu poslati.");
            }
        }
    }
}

void app_main(void)
{
    esp_err_t err;

    err = initialize_nvs();

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "NVS je inicijalizovan.");
    }
    else
    {
        ESP_LOGE(TAG, "Ne mogu da inicijalizujem NVS.");
        return;
    }

    ESP_ERROR_CHECK(wifi_manager_init());
    ESP_ERROR_CHECK(wifi_manager_start());
    ESP_LOGI(TAG, "Cekam Wi-Fi konekciju...");

    err = wifi_manager_wait_connected(15000);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Wi-Fi je povezan. Uredjaj ima IP adresu.");
    }
    else
    {
        ESP_LOGW(TAG,
                 "Wi-Fi jos nije povezan. Aplikacija nastavlja rad.");
    }

    dht11_init();

    ESP_LOGI(TAG, "DHT11 senzor je inicijalizovan.");

    sensor_queue = xQueueCreate(1, sizeof(sensor_data_t));

    if (sensor_queue == NULL)
    {
        ESP_LOGE(TAG, "Ne mogu da kreiram red za podatke.");
        return;
    }

    BaseType_t sensor_task_status = xTaskCreate(
        sensor_task,
        "sensor_task",
        4096,
        NULL,
        5,
        NULL);

    if (sensor_task_status != pdPASS)
    {
        ESP_LOGE(TAG, "Ne mogu da kreiram sensor_task.");
        vQueueDelete(sensor_queue);
        sensor_queue = NULL;
        return;
    }

    BaseType_t http_task_status = xTaskCreate(
        http_task,
        "http_task",
        8192,
        NULL,
        4,
        NULL);

    if (http_task_status != pdPASS)
    {
        ESP_LOGE(TAG, "Ne mogu da kreiram http_task.");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "FreeRTOS zadaci su uspesno pokrenuti.");
}