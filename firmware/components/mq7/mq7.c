#include "mq7.h"

#include <math.h>

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "sdkconfig.h"

#define MQ7_SUPPLY_MV     5000.0f // napajanje modula (VC)
#define MQ7_ADC_SAMPLES   16

// Kriva iz datasheet-a MQ-7 (CO), aproksimirana sa ppm = A * (Rs/R0)^B.
// R0 je otpornost senzora na 100 ppm CO.
#define MQ7_CURVE_A       99.042f
#define MQ7_CURVE_B       -1.518f

// PWM grejaca: 10-bitna rezolucija, 100% = 1024.
#define HEATER_LEDC_MODE      LEDC_LOW_SPEED_MODE
#define HEATER_LEDC_TIMER     LEDC_TIMER_0
#define HEATER_LEDC_CHANNEL   LEDC_CHANNEL_0
#define HEATER_LEDC_RES       LEDC_TIMER_10_BIT
#define HEATER_LEDC_FREQ_HZ   1000
#define HEATER_DUTY_FULL      1024
// Efektivni napon 1,4 V od 5 V: 1024 * 1,4 / 5 = 287
#define HEATER_DUTY_LOW       287

static const char *TAG = "MQ7";

static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t cali_handle = NULL;
static adc_channel_t adc_channel;
static float r0 = CONFIG_MQ7_R0_OHM;

static esp_err_t heater_init(void)
{
#ifdef CONFIG_MQ7_HEATER_CYCLE
    ledc_timer_config_t timer = {
        .speed_mode = HEATER_LEDC_MODE,
        .timer_num = HEATER_LEDC_TIMER,
        .duty_resolution = HEATER_LEDC_RES,
        .freq_hz = HEATER_LEDC_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };

    esp_err_t err = ledc_timer_config(&timer);

    if (err != ESP_OK) {
        return err;
    }

    ledc_channel_config_t channel = {
        .gpio_num = CONFIG_MQ7_HEATER_GPIO,
        .speed_mode = HEATER_LEDC_MODE,
        .channel = HEATER_LEDC_CHANNEL,
        .timer_sel = HEATER_LEDC_TIMER,
        .duty = HEATER_DUTY_FULL,
        .hpoint = 0,
    };

    err = ledc_channel_config(&channel);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Grejac na GPIO%d (PWM, grejni ciklus ukljucen)",
                 CONFIG_MQ7_HEATER_GPIO);
    }

    return err;
#else
    ESP_LOGI(TAG, "Grejni ciklus iskljucen - grejac stalno na 5 V");
    return ESP_OK;
#endif
}

esp_err_t mq7_set_heater(mq7_heater_t level)
{
#ifdef CONFIG_MQ7_HEATER_CYCLE
    uint32_t duty = (level == MQ7_HEATER_HIGH) ? HEATER_DUTY_FULL : HEATER_DUTY_LOW;

    esp_err_t err = ledc_set_duty(HEATER_LEDC_MODE, HEATER_LEDC_CHANNEL, duty);

    if (err == ESP_OK) {
        err = ledc_update_duty(HEATER_LEDC_MODE, HEATER_LEDC_CHANNEL);
    }

    return err;
#else
    (void)level;
    return ESP_OK;
#endif
}

esp_err_t mq7_init(uint32_t r0_ohm)
{
    mq7_set_r0(r0_ohm);

    adc_unit_t unit;
    esp_err_t err = adc_oneshot_io_to_channel(CONFIG_MQ7_ADC_GPIO, &unit, &adc_channel);

    if (err != ESP_OK || unit != ADC_UNIT_1) {
        ESP_LOGE(TAG, "GPIO%d nije pin ADC1", CONFIG_MQ7_ADC_GPIO);
        return ESP_ERR_INVALID_ARG;
    }

    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
    };

    err = adc_oneshot_new_unit(&unit_cfg, &adc_handle);

    if (err != ESP_OK) {
        return err;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,      // opseg do ~3,1 V
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    err = adc_oneshot_config_channel(adc_handle, adc_channel, &chan_cfg);

    if (err != ESP_OK) {
        return err;
    }

    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .chan = adc_channel,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    err = adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "ADC kalibracija nije dostupna (%s), napon ce biti manje tacan",
                 esp_err_to_name(err));
        cali_handle = NULL;
    }

    ESP_LOGI(TAG, "MQ-7 na GPIO%d (ADC1 kanal %d), R0 = %.0f ohm",
             CONFIG_MQ7_ADC_GPIO, adc_channel, (double)r0);

    return heater_init();
}

void mq7_set_r0(uint32_t r0_ohm)
{
    if (r0_ohm > 0) {
        r0 = (float)r0_ohm;
    }
}

float mq7_ppm_from_rs(float rs_ohm, float r0_ohm)
{
    if (rs_ohm <= 0.0f || r0_ohm <= 0.0f) {
        return 0.0f;
    }

    return MQ7_CURVE_A * powf(rs_ohm / r0_ohm, MQ7_CURVE_B);
}

esp_err_t mq7_read(mq7_reading_t *out)
{
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (adc_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    int sum_mv = 0;

    for (int i = 0; i < MQ7_ADC_SAMPLES; i++) {
        int raw;
        esp_err_t err = adc_oneshot_read(adc_handle, adc_channel, &raw);

        if (err != ESP_OK) {
            return err;
        }

        int mv;

        if (cali_handle != NULL) {
            err = adc_cali_raw_to_voltage(cali_handle, raw, &mv);

            if (err != ESP_OK) {
                return err;
            }
        } else {
            // Grubo: 12 bita, ~3100 mV pun opseg.
            mv = raw * 3100 / 4095;
        }

        sum_mv += mv;
    }

    out->adc_mv = sum_mv / MQ7_ADC_SAMPLES;
    out->sensor_mv = out->adc_mv * CONFIG_MQ7_DIVIDER_RATIO_X1000 / 1000;

    // Modul: VC -- Rs -- AOUT -- RL -- GND  =>  Rs = RL * (VC - Vout) / Vout
    float vout = (float)out->sensor_mv;

    if (vout < 1.0f) {
        vout = 1.0f; // izbegava deljenje nulom kada senzor nije povezan
    }

    out->rs_ohm = (float)CONFIG_MQ7_RL_OHM * (MQ7_SUPPLY_MV - vout) / vout;
    out->ppm = mq7_ppm_from_rs(out->rs_ohm, r0);

    return ESP_OK;
}
