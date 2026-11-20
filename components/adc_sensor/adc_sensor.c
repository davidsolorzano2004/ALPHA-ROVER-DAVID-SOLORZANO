#include "adc_sensor.h"

#include <math.h>
#include <stdbool.h>

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ADC_SENSOR_MAX_CHANNELS 10

// Con ADC_ATTEN_DB_12 el ADC se satura cerca de 3.1 V (raw ~4095).
// Si el promedio llega a este valor, la lectura ya no es confiable.
#define ADC_SENSOR_RAW_SATURADO 4085

// Conversión aproximada cuentas -> mV para señales pequeñas (rango útil ~3.1 V en 12 bits)
#define ADC_MV_POR_CUENTA (3100.0 / 4095.0)

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

// ----------------------------------------------------------------------------
// Voltaje REAL de la batería (en voltios).
//   factor_divisor = V_bateria / V_pin  (relación del divisor resistivo)
//   Si conoces las resistencias: factor = (R_alta + R_baja) / R_baja
//   Si no, se calibra con un multímetro: factor = V_real_medido / V_leido_en_el_pin
// ----------------------------------------------------------------------------
float adc_sensor_read_battery_v(adc_channel_t channel, int num_muestras, float factor_divisor)
{
    int raw_promedio = 0;
    int mv_pin = adc_sensor_read_voltage_mv(channel, num_muestras, &raw_promedio);

    if (raw_promedio >= ADC_SENSOR_RAW_SATURADO) {
        ESP_LOGW(TAG, "ADC canal %d SATURADO (raw=%d, %d mV): el pin recibe mas de ~3.1 V. "
                      "Cambia el divisor de voltaje; esta lectura NO es confiable",
                 channel, raw_promedio, mv_pin);
    }

    return (mv_pin / 1000.0f) * factor_divisor;
}

// ----------------------------------------------------------------------------
// MICRÓFONO
// Mide durante una ventana de TIEMPO fija (ventana_ms), sin pausas, y calcula
// el RMS de la parte alterna de la señal (se descarta el nivel DC del módulo).
// Devuelve la amplitud pico a pico EQUIVALENTE en mV (2*sqrt(2)*RMS).
// El RMS es mucho más estable que max-min con el ruido del ADC del ESP32, que
// con pocas muestras daba lecturas casi iguales en silencio y con sonido.
// El segundo parámetro ahora es la duración de la ventana en milisegundos (antes era un número de muestras).
// raw_pp_out devuelve el pico a pico crudo (max-min) solo para depuración.
// ----------------------------------------------------------------------------
int adc_sensor_read_mic_amplitude_mv(adc_channel_t channel, int ventana_ms, int *raw_pp_out)
{
    if (ventana_ms < 5) ventana_ms = 5;

    int raw = 0;
    int raw_max = 0;
    int raw_min = 4095;
    int64_t suma = 0;
    int64_t suma_cuad = 0;
    int64_t n = 0;

    const int64_t t_fin = esp_timer_get_time() + (int64_t)ventana_ms * 1000;

    while (esp_timer_get_time() < t_fin) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, channel, &raw));
        suma += raw;
        suma_cuad += (int64_t)raw * raw;
        n++;
        if (raw > raw_max) raw_max = raw;
        if (raw < raw_min) raw_min = raw;
    }

    if (raw_pp_out != NULL) {
        *raw_pp_out = (n > 0) ? (raw_max - raw_min) : 0;
    }

    if (n < 2) {
        return 0;
    }

    double media = (double)suma / (double)n;
    double varianza = (double)suma_cuad / (double)n - media * media;
    if (varianza < 0.0) varianza = 0.0;

    double rms_raw = sqrt(varianza);
    double pp_mv = rms_raw * 2.8284271 * ADC_MV_POR_CUENTA;

    return (int)(pp_mv + 0.5);
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