#pragma once

#include "esp_adc/adc_oneshot.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa la unidad ADC1 y configura + calibra los tres canales
 *        utilizados por la aplicación (dos entradas analógicas y un micrófono).
 */
void adc_sensor_init(adc_channel_t canal_entrada_1, adc_channel_t canal_entrada_2, adc_channel_t canal_mic);

/**
 * @brief Lee el voltaje promedio (mV) de un canal, promediando varias muestras.
 *
 * @param channel           Canal ADC a leer (debe haberse inicializado con adc_sensor_init).
 * @param num_muestras      Número de muestras a promediar.
 * @param raw_promedio_out  Salida opcional con el valor crudo promedio.
 */
int adc_sensor_read_voltage_mv(adc_channel_t channel, int num_muestras, int *raw_promedio_out);

/**
 * @brief Lee la amplitud pico a pico (mV) de un canal (uso típico: micrófono).
 *
 * @param channel      Canal ADC a leer.
 * @param num_muestras Número de muestras a tomar para calcular pico a pico.
 * @param raw_pp_out   Salida opcional con el valor crudo pico a pico.
 */
int adc_sensor_read_mic_amplitude_mv(adc_channel_t channel, int num_muestras, int *raw_pp_out);

/**
 * @brief Clasifica una amplitud de micrófono (mV) según los umbrales dados.
 */
const char *adc_sensor_classify_mic_level(int amplitud_mv, int umbral_silencio, int umbral_bajo, int umbral_medio);

#ifdef __cplusplus
}
#endif
