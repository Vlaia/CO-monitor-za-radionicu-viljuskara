#include "co_logic.h"

#include <stddef.h>

void co_logic_init(co_logic_t *logic, uint16_t alarm_ppm)
{
    logic->alarm_ppm = alarm_ppm;
    logic->level = CO_LEVEL_OK;
}

void co_logic_set_alarm_ppm(co_logic_t *logic, uint16_t alarm_ppm)
{
    logic->alarm_ppm = alarm_ppm;
}

bool co_logic_update(co_logic_t *logic, float ppm)
{
    // Osnovni prag (1. faza): alarm na/iznad praga, gasi se ispod praga.
    // Histereza i prag upozorenja dolaze u 2. fazi.
    co_level_t new_level = (ppm >= (float)logic->alarm_ppm) ? CO_LEVEL_ALARM : CO_LEVEL_OK;

    if (new_level == logic->level) {
        return false;
    }

    logic->level = new_level;
    return true;
}

const char *co_level_name(co_level_t level)
{
    switch (level) {
    case CO_LEVEL_OK:
        return "ok";
    case CO_LEVEL_ALARM:
        return "alarm";
    default:
        return "nepoznato";
    }
}
