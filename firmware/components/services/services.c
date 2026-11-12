#include <stdio.h>
#include "services.h"
#include "mqtt_link.h"
#include "esp_log.h"

static const char *TAG = "SERVICES";

#define JSON_MAX_LEN 160

// Dodaje "ts" samo ako je vreme poznato.
static int append_ts(char *buf, size_t len, time_t ts)
{
    if (ts > 0) {
        return snprintf(buf, len, ",\"ts\":%lld}", (long long)ts);
    }
    return snprintf(buf, len, "}");
}

static esp_err_t finish_and_send(char *json, int length, time_t ts,
                                 const char *subtopic, int qos)
{
    if (length < 0 || length >= JSON_MAX_LEN) {
        ESP_LOGE(TAG, "Greska pri formiranju JSON podataka");
        return ESP_FAIL;
    }

    int tail = append_ts(json + length, JSON_MAX_LEN - length, ts);

    if (tail < 0 || length + tail >= JSON_MAX_LEN) {
        ESP_LOGE(TAG, "Greska pri formiranju JSON podataka");
        return ESP_FAIL;
    }

    return mqtt_link_publish(subtopic, json, qos, false);
}

esp_err_t send_co_measurement(float ppm, int sensor_mv, time_t ts)
{
    char json[JSON_MAX_LEN];

    int length = snprintf(json, sizeof(json),
                          "{\"ppm\":%.1f,\"mv\":%d",
                          (double)ppm, sensor_mv);

    return finish_and_send(json, length, ts, "co", 0);
}

esp_err_t send_motion_detected(bool motion, time_t ts)
{
    char json[JSON_MAX_LEN];

    int length = snprintf(json, sizeof(json),
                          "{\"motion\":%s",
                          motion ? "true" : "false");

    return finish_and_send(json, length, ts, "pokret", 1);
}

esp_err_t send_alarm(const char *source, bool active, float ppm, time_t ts)
{
    char json[JSON_MAX_LEN];

    int length = snprintf(json, sizeof(json),
                          "{\"source\":\"%s\",\"active\":%s,\"ppm\":%.1f",
                          source, active ? "true" : "false", (double)ppm);

    return finish_and_send(json, length, ts, "alarm", 1);
}
