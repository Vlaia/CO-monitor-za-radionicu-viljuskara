#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "esp_err.h"

esp_err_t https_client_send_sensor_data(char *json_data, int length);

#endif