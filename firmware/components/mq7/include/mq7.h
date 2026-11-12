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

esp_err_t mq7_init(uint32_t r0_ohm);

// Uprosecava nekoliko ADC odbiraka i racuna ppm.
esp_err_t mq7_read(mq7_reading_t *out);

void mq7_set_r0(uint32_t r0_ohm);

// Racunanje ppm iz otpornosti (izdvojeno radi testiranja).
float mq7_ppm_from_rs(float rs_ohm, float r0_ohm);

#endif
