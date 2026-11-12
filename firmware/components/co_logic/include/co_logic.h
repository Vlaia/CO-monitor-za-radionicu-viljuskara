#ifndef CO_LOGIC_H
#define CO_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

// Logika praga za CO, odvojena od hardvera da bi se lako prosirila
// (upozorenje, histereza) i testirala.

typedef enum
{
    CO_LEVEL_OK = 0,
    CO_LEVEL_ALARM,
} co_level_t;

typedef struct
{
    uint16_t alarm_ppm;
    co_level_t level;
} co_logic_t;

void co_logic_init(co_logic_t *logic, uint16_t alarm_ppm);
void co_logic_set_alarm_ppm(co_logic_t *logic, uint16_t alarm_ppm);

// Obradjuje novo merenje; vraca true ako se nivo promenio.
bool co_logic_update(co_logic_t *logic, float ppm);

const char *co_level_name(co_level_t level);

#endif
