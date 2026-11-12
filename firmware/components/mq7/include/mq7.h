#ifndef MQ7_H
#define MQ7_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

typedef struct
{
    int adc_mv;      // napon na ADC pinu (posle delitelja)
    int sensor_mv;   // napon na AOUT izlazu modula
    float rs_ohm;    // otpornost senzora
    float ppm;       // priblizna koncentracija CO
} mq7_reading_t;

typedef enum
{
    MQ7_HEATER_HIGH, // 5 V - zagrevanje / ciscenje senzora
    MQ7_HEATER_LOW,  // ~1,4 V - faza merenja
} mq7_heater_t;

esp_err_t mq7_init(uint32_t r0_ohm);

// Postavlja napon grejaca (PWM preko MOSFET-a). Bez grejnog ciklusa ne radi nista.
esp_err_t mq7_set_heater(mq7_heater_t level);

// Uprosecava nekoliko ADC odbiraka i racuna ppm.
esp_err_t mq7_read(mq7_reading_t *out);

void mq7_set_r0(uint32_t r0_ohm);

// Racunanje ppm iz otpornosti (izdvojeno radi testiranja).
float mq7_ppm_from_rs(float rs_ohm, float r0_ohm);

#endif
