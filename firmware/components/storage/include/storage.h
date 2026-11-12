#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>
#include "esp_err.h"

// Komponenta "nvs" sa vezbi, preimenovana u "storage" jer se njen nvs.h
// sudarao sa nvs.h iz ESP-IDF-a. Pored inicijalizacije NVS-a sada cuva
// WiFi kredencijale i podesavanja uredjaja, pa prezivljavaju restart.

#define STORAGE_SSID_MAX_LEN     32
#define STORAGE_PASSWORD_MAX_LEN 64

typedef struct
{
    char ssid[STORAGE_SSID_MAX_LEN + 1];
    char password[STORAGE_PASSWORD_MAX_LEN + 1];
} wifi_credentials_t;

typedef struct
{
    uint16_t co_alarm_ppm; // prag alarma za CO
    uint32_t mq7_r0_ohm;   // kalibracija MQ-7 senzora
} device_settings_t;

esp_err_t initialize_nvs(void);

// ESP_ERR_NVS_NOT_FOUND ako kredencijali jos nisu upisani.
esp_err_t storage_get_wifi(wifi_credentials_t *out);
esp_err_t storage_set_wifi(const char *ssid, const char *password);

// Popunjava samo polja koja postoje u NVS-u; ostala ostaju kako ih je
// pozivalac postavio (podrazumevane vrednosti).
esp_err_t storage_load_settings(device_settings_t *settings);
esp_err_t storage_save_settings(const device_settings_t *settings);

#endif
