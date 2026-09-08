#include "adc_sensor.h"

#include <stdbool.h>

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ADC_SENSOR_MAX_CHANNELS 10

static const char *TAG = "MQTT_SISTEMA";

static adc_oneshot_unit_handle_t adc1_handle;

// Estado de calibración indexado por número de canal ADC (ADC_CHANNEL_x)
static adc_cali_handle_t cali_handle_por_canal[ADC_SENSOR_MAX_CHANNELS] = {NULL};
static bool cali_habilitada_por_canal[ADC_SENSOR_MAX_CHANNELS] = {false};

// ============================================================================
// CALIBRACIÓN ADC
// ============================================================================
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .chan = channel,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibración ADC habilitada para canal %d", channel);
        return true;
    }
    ESP_LOGW(TAG, "Calibración ADC no disponible para canal %d, se usará valor crudo", channel);
    return false;
}

static void configurar_canal(adc_channel_t channel)
{
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, channel, &chan_config));

    adc_cali_handle_t handle = NULL;
    bool habilitada = adc_calibration_init(ADC_UNIT_1, channel, ADC_ATTEN_DB_12, &handle);

    cali_handle_por_canal[channel] = handle;
    cali_habilitada_por_canal[channel] = habilitada;
}

// ============================================================================
// API PÚBLICA
// ============================================================================
void adc_sensor_init(adc_channel_t canal_entrada_1, adc_channel_t canal_entrada_2, adc_channel_t canal_mic)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    configurar_canal(canal_entrada_1);
    configurar_canal(canal_entrada_2);
    configurar_canal(canal_mic);
}

int adc_sensor_read_voltage_mv(adc_channel_t channel, int num_muestras, int *raw_promedio_out)
{
    int64_t suma_raw = 0;
    int raw = 0;
    int voltage_mv = 0;

    for (int i = 0; i < num_muestras; i++) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, channel, &raw));
        suma_raw += raw;
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    int raw_promedio = (int)(suma_raw / num_muestras);

    if (raw_promedio_out != NULL) {
        *raw_promedio_out = raw_promedio;
    }

    if (cali_habilitada_por_canal[channel]) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle_por_canal[channel], raw_promedio, &voltage_mv));
    } else {
        voltage_mv = (raw_promedio * 3300) / 4095;
    }

    return voltage_mv;
}

int adc_sensor_read_mic_amplitude_mv(adc_channel_t channel, int num_muestras, int *raw_pp_out)
{
    int raw = 0;
    int raw_max = 0;
    int raw_min = 4095;

    for (int i = 0; i < num_muestras; i++) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, channel, &raw));
        if (raw > raw_max) raw_max = raw;
        if (raw < raw_min) raw_min = raw;
    }

    int raw_pp = raw_max - raw_min;

    if (raw_pp_out != NULL) {
        *raw_pp_out = raw_pp;
    }

    int amplitud_mv = 0;

    if (cali_habilitada_por_canal[channel]) {
        int mv_max = 0, mv_min = 0;
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle_por_canal[channel], raw_max, &mv_max));
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle_por_canal[channel], raw_min, &mv_min));
        amplitud_mv = mv_max - mv_min;
    } else {
        amplitud_mv = (raw_pp * 3300) / 4095;
    }

    return amplitud_mv;
}

const char *adc_sensor_classify_mic_level(int amplitud_mv, int umbral_silencio, int umbral_bajo, int umbral_medio)
{
    if (amplitud_mv < umbral_silencio) {
        return "silencio";
    } else if (amplitud_mv < umbral_bajo) {
        return "bajo";
    } else if (amplitud_mv < umbral_medio) {
        return "medio";
    } else {
        return "alto";
    }
}
