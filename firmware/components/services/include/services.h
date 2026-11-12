#ifndef SERVICES_H
#define SERVICES_H

#include <stdbool.h>
#include <time.h>
#include "esp_err.h"

// Formiranje JSON poruka i slanje na MQTT teme (na vezbama je isti sloj
// slao podatke preko HTTP-a). `ts` je Unix vreme dogadjaja; 0 znaci da sat
// jos nije sinhronizovan, pa server koristi vreme prijema.

// radiona/<id>/co - periodicno merenje (QoS 0)
esp_err_t send_co_measurement(float ppm, int sensor_mv, time_t ts);

// radiona/<id>/pokret - samo kada se desi (QoS 1)
esp_err_t send_motion_detected(bool motion, time_t ts);

// radiona/<id>/alarm - ukljucenje/iskljucenje alarma (QoS 1)
// source: "co" ili "pokret"
esp_err_t send_alarm(const char *source, bool active, float ppm, time_t ts);

#endif
