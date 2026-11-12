#include <stdio.h>
#include "services.h"
#include "http_client.h"
#include "esp_log.h"

#define DEVICE_LICENCE "ESP32-LICENSE-001"

static const char *TAG = "SERVICES";

esp_err_t send_temperature_and_humidity(float temperature, float humidity)
{

    char json_data[128];

    int length = snprintf(
        json_data,
        sizeof(json_data),
        "{\"licence\":\"%s\","
        "\"sensorType\":%d,"
        "\"temperature\":%.1f,"
        "\"humidity\":%.1f}",
        DEVICE_LICENCE,
        0,
        (double)temperature,
        (double)humidity);

    if (length < 0 || length >= sizeof(json_data))
    {
        ESP_LOGE(TAG, "Greska pri formiranju JSON podataka");
        return ESP_FAIL;
    }

    return https_client_send_sensor_data(json_data, length);
}
