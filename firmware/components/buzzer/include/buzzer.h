#ifndef BUZZER_H
#define BUZZER_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

// Inicijalizuje pin i pokrece task zujalice.
esp_err_t buzzer_init(void);

// Direktno upravljanje (kao na vezbama).
esp_err_t buzzer_on(void);
esp_err_t buzzer_off(void);

// Alarm: isprekidan zvuk dok se ne ugasi.
void buzzer_set_alarm(bool active);
bool buzzer_alarm_active(void);

// Kratki pisak `count` puta (npr. na pokret). Ne blokira.
void buzzer_chirp(uint8_t count);

#endif
