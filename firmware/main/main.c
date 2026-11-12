#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_netif_sntp.h"
#include "sdkconfig.h"

#include "storage.h"
#include "wifi_manager.h"
#include "mqtt_link.h"
#include "services.h"
#include "buzzer.h"
#include "pir.h"
#include "mq7.h"
#include "co_logic.h"

static const char *TAG = "MAIN";

#define VALID_EPOCH 1700000000 // manje od ovoga = sat nije sinhronizovan

// Dogadjaji koje taskovi senzora salju tasku za slanje telemetrije.
typedef enum
{
    EV_CO_SAMPLE,
    EV_CO_ALARM,
    EV_MOTION,
} app_event_type_t;

typedef struct
{
    app_event_type_t type;
    time_t ts;
    float ppm;
    int sensor_mv;
    bool active; // EV_CO_ALARM: alarm ukljucen; EV_MOTION: pocetak pokreta
} app_event_t;

static QueueHandle_t event_queue = NULL;
static QueueHandle_t pir_queue = NULL;

static device_settings_t settings = {
    .co_alarm_ppm = CONFIG_CO_ALARM_PPM,
    .mq7_r0_ohm = CONFIG_MQ7_R0_OHM,
};

static time_t now_ts(void)
{
    time_t now = time(NULL);
    return now > VALID_EPOCH ? now : 0;
}

static void post_event(const app_event_t *ev, TickType_t wait)
{
    if (xQueueSend(event_queue, ev, wait) != pdTRUE) {
        ESP_LOGW(TAG, "Red dogadjaja je pun, dogadjaj %d odbacen", ev->type);
    }
}

// ---------------------------------------------------------------- CO senzor

static void measure_co(co_logic_t *logic)
{
    mq7_reading_t reading;
    esp_err_t err = mq7_read(&reading);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Greska pri citanju MQ-7: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "CO: %.1f ppm (AOUT %d mV, Rs %.0f ohm)",
             (double)reading.ppm, reading.sensor_mv, (double)reading.rs_ohm);

    time_t ts = now_ts();

    app_event_t sample = {
        .type = EV_CO_SAMPLE,
        .ts = ts,
        .ppm = reading.ppm,
        .sensor_mv = reading.sensor_mv,
    };

    // Periodicno merenje: ako je red pun, novo merenje se odbacuje.
    post_event(&sample, 0);

    if (co_logic_update(logic, reading.ppm)) {
        bool alarm = logic->level == CO_LEVEL_ALARM;

        ESP_LOGW(TAG, "CO nivo: %s (%.1f ppm, prag %u ppm)",
                 co_level_name(logic->level), (double)reading.ppm, logic->alarm_ppm);

        buzzer_set_alarm(alarm);

        app_event_t ev = {
            .type = EV_CO_ALARM,
            .ts = ts,
            .ppm = reading.ppm,
            .active = alarm,
        };

        post_event(&ev, pdMS_TO_TICKS(100));
    }
}

static void co_task(void *pvParameters)
{
    (void)pvParameters;

    co_logic_t logic;
    co_logic_init(&logic, settings.co_alarm_ppm);

    const TickType_t sample_period = pdMS_TO_TICKS(CONFIG_MQ7_SAMPLE_PERIOD_MS);

#ifdef CONFIG_MQ7_HEATER_CYCLE
    // Ciklus iz datasheet-a: 60 s na 5 V (cisti senzor), pa 90 s na 1,4 V kada se meri.
    // Alarm zadrzava stanje i za vreme grejanja.
    while (1) {
        ESP_LOGI(TAG, "MQ-7: faza grejanja (%d s)", CONFIG_MQ7_HEAT_PHASE_S);
        mq7_set_heater(MQ7_HEATER_HIGH);
        vTaskDelay(pdMS_TO_TICKS(CONFIG_MQ7_HEAT_PHASE_S * 1000));

        ESP_LOGI(TAG, "MQ-7: faza merenja (%d s)", CONFIG_MQ7_MEASURE_PHASE_S);
        mq7_set_heater(MQ7_HEATER_LOW);

        TickType_t phase_end = xTaskGetTickCount() +
                               pdMS_TO_TICKS(CONFIG_MQ7_MEASURE_PHASE_S * 1000);

        vTaskDelay(pdMS_TO_TICKS(CONFIG_MQ7_SETTLE_S * 1000));

        while ((int32_t)(phase_end - xTaskGetTickCount()) > 0) {
            measure_co(&logic);
            vTaskDelay(sample_period);
        }
    }
#else
    ESP_LOGI(TAG, "MQ-7: zagrevanje %d s", CONFIG_MQ7_WARMUP_S);
    vTaskDelay(pdMS_TO_TICKS(CONFIG_MQ7_WARMUP_S * 1000));

    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        measure_co(&logic);
        vTaskDelayUntil(&last_wake, sample_period);
    }
#endif
}

// ---------------------------------------------------------------- PIR

static void pir_task(void *pvParameters)
{
    (void)pvParameters;

    pir_edge_t edge;
    bool last_motion = false;

    while (1) {
        if (xQueueReceive(pir_queue, &edge, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        // Smetnje mogu da daju vise prekida za isti nivo.
        if (edge.motion == last_motion) {
            continue;
        }

        last_motion = edge.motion;

        ESP_LOGI(TAG, "%s", edge.motion ? "Detektovan pokret!" : "Pokret prestao.");

        if (edge.motion) {
            // Ponasanje sa vezbi: zujalica se oglasi na pokret.
            buzzer_chirp(3);
        }

        app_event_t ev = {
            .type = EV_MOTION,
            .ts = now_ts(),
            .active = edge.motion,
        };

        post_event(&ev, pdMS_TO_TICKS(100));
    }
}

// ---------------------------------------------------------------- telemetrija

static void telemetry_task(void *pvParameters)
{
    (void)pvParameters;

    app_event_t ev;

    while (1) {
        if (xQueueReceive(event_queue, &ev, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        esp_err_t err = ESP_OK;

        switch (ev.type) {
        case EV_CO_SAMPLE:
            if (!mqtt_link_is_connected()) {
                ESP_LOGW(TAG, "Nema veze sa brokerom, merenje nije poslato");
                continue;
            }
            err = send_co_measurement(ev.ppm, ev.sensor_mv, ev.ts);
            break;

        case EV_CO_ALARM:
            err = send_alarm("co", ev.active, ev.ppm, ev.ts);
            break;

        case EV_MOTION:
            err = send_motion_detected(ev.active, ev.ts);
            break;
        }

        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Slanje dogadjaja %d nije uspelo: %s",
                     ev.type, esp_err_to_name(err));
        }
    }
}

// ---------------------------------------------------------------- init

static void time_init(void)
{
    setenv("TZ", CONFIG_TIMEZONE, 1);
    tzset();

    esp_sntp_config_t sntp_cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_err_t err = esp_netif_sntp_init(&sntp_cfg);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SNTP nije pokrenut: %s", esp_err_to_name(err));
        return;
    }

    if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) == ESP_OK) {
        ESP_LOGI(TAG, "Vreme sinhronizovano");
    } else {
        ESP_LOGW(TAG, "Vreme jos nije sinhronizovano, server ce koristiti vreme prijema");
    }
}

static bool start_task(TaskFunction_t fn, const char *name, uint32_t stack, UBaseType_t prio)
{
    if (xTaskCreate(fn, name, stack, NULL, prio, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Ne mogu da kreiram %s", name);
        return false;
    }
    return true;
}

void app_main(void)
{
    esp_err_t err = initialize_nvs();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Ne mogu da inicijalizujem NVS: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "NVS je inicijalizovan.");

    // Pragovi iz NVS-a; pri prvom pokretanju upisuju se podrazumevane vrednosti.
    storage_load_settings(&settings);
    storage_save_settings(&settings);
    ESP_LOGI(TAG, "Prag alarma: %u ppm, R0: %lu ohm",
             settings.co_alarm_ppm, (unsigned long)settings.mq7_r0_ohm);

    ESP_ERROR_CHECK(buzzer_init());

    err = wifi_manager_init();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi nije inicijalizovan: %s", esp_err_to_name(err));
        return;
    }

    ESP_ERROR_CHECK(wifi_manager_start());
    ESP_LOGI(TAG, "Cekam Wi-Fi konekciju...");

    if (wifi_manager_wait_connected(15000) == ESP_OK) {
        ESP_LOGI(TAG, "Wi-Fi je povezan.");
        time_init();
    } else {
        ESP_LOGW(TAG, "Wi-Fi jos nije povezan. Aplikacija nastavlja rad.");
    }

    ESP_ERROR_CHECK(mqtt_link_start());

    event_queue = xQueueCreate(16, sizeof(app_event_t));
    pir_queue = xQueueCreate(8, sizeof(pir_edge_t));

    if (event_queue == NULL || pir_queue == NULL) {
        ESP_LOGE(TAG, "Ne mogu da kreiram redove.");
        return;
    }

    ESP_ERROR_CHECK(mq7_init(settings.mq7_r0_ohm));
    ESP_ERROR_CHECK(pir_init(pir_queue));

    if (!start_task(telemetry_task, "telemetry_task", 4096, 4) ||
        !start_task(co_task, "co_task", 4096, 5) ||
        !start_task(pir_task, "pir_task", 3072, 5)) {
        return;
    }

    ESP_LOGI(TAG, "FreeRTOS zadaci su uspesno pokrenuti.");
}
