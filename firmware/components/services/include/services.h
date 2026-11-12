#ifndef SERVICES_H
#define SERVICES_H

#include "esp_err.h"

esp_err_t send_temperature_and_humidity(float temperature, float humidity);
esp_err_t send_motion_detected();

#endif