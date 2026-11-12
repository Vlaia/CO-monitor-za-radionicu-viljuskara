#include "http_client.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"

static const char *TAG = "HTTPS_CLIENT";

#define HTTPS_SERVER_URL "https://stupid-colts-notice.loca.lt/device/sensor-telemetry"

esp_err_t https_client_send_sensor_data(char *json_data, int length)
{
    esp_http_client_config_t config = {
        .url = HTTPS_SERVER_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 20000,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .keep_alive_enable = false,
        .buffer_size = 4096,
        .buffer_size_tx = 2048,
    };

    ESP_LOGI(TAG, "JSON koji se salje: %s", json_data);
    
    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (client == NULL)
    {
        ESP_LOGE(TAG, "Ne mogu da kreiram HTTP klijent");
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "ngrok-skip-browser-warning", "true");
    esp_http_client_set_header(client, "User-Agent", "ESP32S3");

    esp_err_t err = esp_http_client_set_post_field(client, json_data, length);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Saljem podatke: %s", json_data);
        err = esp_http_client_perform(client);
    }

    if (err == ESP_OK)
    {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP status: %d", status_code);

        if (status_code >= 200 && status_code < 300)
        {
            ESP_LOGI(TAG, "Podaci su uspesno poslati");
        }
        else
        {
            ESP_LOGE(TAG, "Server je vratio HTTP status %d", status_code);
            err = ESP_FAIL;
        }
    }
    else
    {
        ESP_LOGE(TAG, "HTTPS zahtev nije uspeo: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}