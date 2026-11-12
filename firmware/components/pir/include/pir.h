#ifndef PIR_H
#define PIR_H

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_err.h"

// Poruka koju prekidna rutina salje u red pri svakoj promeni nivoa PIR izlaza.
typedef struct
{
    bool motion;          // true = pocetak pokreta, false = kraj
    TickType_t tick;      // trenutak promene
} pir_edge_t;

// Konfigurise PIR pin sa prekidom na obe ivice. Promene se salju u `edge_queue`
// (red elemenata tipa pir_edge_t).
esp_err_t pir_init(QueueHandle_t edge_queue);

bool pir_motion_detected(void);

#endif
