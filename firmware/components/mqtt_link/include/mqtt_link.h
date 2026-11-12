#ifndef MQTT_LINK_H
#define MQTT_LINK_H

#include <stdbool.h>
#include "esp_err.h"

// Veza sa MQTT brokerom. Teme su oblika <prefiks>/<id uredjaja>/<podtema>,
// npr. radiona/radiona1/co. Na temi "status" se drzi retained poruka
// {"online":true/false}; false salje broker (Last Will) kada uredjaj nestane.

esp_err_t mqtt_link_start(void);
bool mqtt_link_is_connected(void);

// qos 0: salje se samo ako je veza aktivna (periodicna merenja).
// qos 1: ide u outbox i salje se i posle ponovnog povezivanja (dogadjaji).
esp_err_t mqtt_link_publish(const char *subtopic, const char *json, int qos, bool retain);

#endif
