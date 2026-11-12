#include "storage.h"

#include <string.h>

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

#define NS_WIFI     "wifi"
#define NS_SETTINGS "settings"

#define KEY_SSID     "ssid"
#define KEY_PASSWORD "pass"
#define KEY_ALARM    "co_alarm"
#define KEY_R0       "mq7_r0"

static const char *TAG = "STORAGE";

esp_err_t initialize_nvs(void)
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        err = nvs_flash_erase();

        if (err != ESP_OK)
        {
            return err;
        }

        err = nvs_flash_init();
    }

    return err;
}

esp_err_t storage_get_wifi(wifi_credentials_t *out)
{
    if (out == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(out, 0, sizeof(*out));

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NS_WIFI, NVS_READONLY, &handle);

    if (err != ESP_OK)
    {
        // Namespace ne postoji dok se prvi put ne upise.
        return err == ESP_ERR_NVS_NOT_FOUND ? ESP_ERR_NVS_NOT_FOUND : err;
    }

    size_t len = sizeof(out->ssid);
    err = nvs_get_str(handle, KEY_SSID, out->ssid, &len);

    if (err == ESP_OK)
    {
        len = sizeof(out->password);
        err = nvs_get_str(handle, KEY_PASSWORD, out->password, &len);

        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            // Otvorena mreza bez lozinke.
            out->password[0] = '\0';
            err = ESP_OK;
        }
    }

    nvs_close(handle);

    if (err == ESP_OK && out->ssid[0] == '\0')
    {
        err = ESP_ERR_NVS_NOT_FOUND;
    }

    return err;
}

esp_err_t storage_set_wifi(const char *ssid, const char *password)
{
    if (ssid == NULL || password == NULL ||
        strlen(ssid) == 0 ||
        strlen(ssid) > STORAGE_SSID_MAX_LEN ||
        strlen(password) > STORAGE_PASSWORD_MAX_LEN)
    {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NS_WIFI, NVS_READWRITE, &handle);

    if (err != ESP_OK)
    {
        return err;
    }

    err = nvs_set_str(handle, KEY_SSID, ssid);

    if (err == ESP_OK)
    {
        err = nvs_set_str(handle, KEY_PASSWORD, password);
    }

    if (err == ESP_OK)
    {
        err = nvs_commit(handle);
    }

    nvs_close(handle);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "WiFi kredencijali upisani u NVS (SSID: %s)", ssid);
    }

    return err;
}

esp_err_t storage_load_settings(device_settings_t *settings)
{
    if (settings == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NS_SETTINGS, NVS_READONLY, &handle);

    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGI(TAG, "Podesavanja nisu u NVS-u, koristim podrazumevana");
        return ESP_OK;
    }

    if (err != ESP_OK)
    {
        return err;
    }

    uint16_t u16;
    uint32_t u32;

    if (nvs_get_u16(handle, KEY_ALARM, &u16) == ESP_OK)
    {
        settings->co_alarm_ppm = u16;
    }

    if (nvs_get_u32(handle, KEY_R0, &u32) == ESP_OK)
    {
        settings->mq7_r0_ohm = u32;
    }

    nvs_close(handle);

    return ESP_OK;
}

esp_err_t storage_save_settings(const device_settings_t *settings)
{
    if (settings == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NS_SETTINGS, NVS_READWRITE, &handle);

    if (err != ESP_OK)
    {
        return err;
    }

    err = nvs_set_u16(handle, KEY_ALARM, settings->co_alarm_ppm);

    if (err == ESP_OK)
    {
        err = nvs_set_u32(handle, KEY_R0, settings->mq7_r0_ohm);
    }

    if (err == ESP_OK)
    {
        err = nvs_commit(handle);
    }

    nvs_close(handle);

    return err;
}
