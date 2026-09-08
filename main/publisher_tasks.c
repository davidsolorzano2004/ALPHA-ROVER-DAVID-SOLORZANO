#include "publisher_tasks.h"
#include "app_config.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h> // Agregado para usar strncpy() con el texto del micrófono

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "mqtt_manager.h"
#include "adc_sensor.h"
#include "dht11_sensor.h"

// Variables globales provenientes de main.c
extern float g_temp;
extern float g_hum;
extern float g_vbat1;
extern float g_vbat2;
extern float g_mic_v;
extern char g_mic_lvl[16];

static const char *TAG = "MQTT_SISTEMA";

// Tarea 1: Estado alternante ("1" / "0")
void mqtt_publisher_task(void *pvParameters)
{
    int estado = 1;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(PUBLISH_PERIOD_MS));

        char payload[2];
        snprintf(payload, sizeof(payload), "%d", estado);

        int msg_id = mqtt_manager_publish(TOPIC_PUB, payload);
        ESP_LOGI(TAG, "Enviado a [%s]: %s (ID msg=%d)", TOPIC_PUB, payload, msg_id);

        estado = !estado;
    }
}

// Tarea 2: Entradas Analógicas + Micrófono
void adc_publisher_task(void *pvParameters)
{
    while (1) {
        int raw1 = 0, raw2 = 0, raw_pp_mic = 0;

        int mv1 = adc_sensor_read_voltage_mv(ADC1_CHANNEL_ENTRADA_1, ADC_NUM_MUESTRAS, &raw1);
        int mv2 = adc_sensor_read_voltage_mv(ADC1_CHANNEL_ENTRADA_2, ADC_NUM_MUESTRAS, &raw2);
        int mv_mic = adc_sensor_read_mic_amplitude_mv(ADC1_CHANNEL_MIC, MIC_NUM_MUESTRAS, &raw_pp_mic);

        float v1 = mv1 / 1000.0f;
        float v2 = mv2 / 1000.0f;
        float v_mic = mv_mic / 1000.0f;

        const char *nivel_mic = adc_sensor_classify_mic_level(
            mv_mic, MIC_UMBRAL_SILENCIO_MV, MIC_UMBRAL_BAJO_MV, MIC_UMBRAL_MEDIO_MV);

        // =========================================================
        // ACTUALIZACIÓN DE VARIABLES GLOBALES PARA EL DASHBOARD
        // =========================================================
        g_vbat1 = v1;
        g_vbat2 = v2;
        g_mic_v = v_mic;
        // Copiamos el texto de forma segura para no desbordar el array de 16 caracteres
        strncpy(g_mic_lvl, nivel_mic, sizeof(g_mic_lvl) - 1);
        g_mic_lvl[sizeof(g_mic_lvl) - 1] = '\0'; 
        // =========================================================

        char payload1[16];
        char payload2[16];
        char payload_mic[64];

        snprintf(payload1, sizeof(payload1), "%.3f", v1);
        snprintf(payload2, sizeof(payload2), "%.3f", v2);
        snprintf(payload_mic, sizeof(payload_mic), "{\"voltaje\":%.3f,\"nivel\":\"%s\"}", v_mic, nivel_mic);

        int id1 = mqtt_manager_publish(TOPIC_PUB_ADC1, payload1);
        int id2 = mqtt_manager_publish(TOPIC_PUB_ADC2, payload2);
        int id_mic = mqtt_manager_publish(TOPIC_PUB_MIC, payload_mic);

        ESP_LOGI(TAG, "ADC1 (GPIO34): %s V (ID=%d) | ADC2 (GPIO35): %s V (ID=%d)", payload1, id1, payload2, id2);
        ESP_LOGI(TAG, "Micrófono (GPIO32): %s (ID=%d)", payload_mic, id_mic);

        vTaskDelay(pdMS_TO_TICKS(ADC_PUBLISH_PERIOD_MS));
    }
}

#define DHT_MAX_REINTENTOS 3
#define DHT_MAX_FALLOS_CONSECUTIVOS 5

// Tarea 3: Sensor DHT11
void dht11_publisher_task(void *pvParameters)
{
    int temperatura = 0;
    int humedad = 0;
    bool hay_lectura_valida = false;
    int fallos_consecutivos = 0;

    char payload_temp[16];
    char payload_hum[16];
    char payload_json[64];

    while (1) {
        esp_err_t resultado = ESP_FAIL;

        for (int intento = 1; intento <= DHT_MAX_REINTENTOS; intento++) {
            resultado = dht11_sensor_read(DHT_PIN, &temperatura, &humedad);
            if (resultado == ESP_OK) {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(DHT_RETRY_DELAY_MS));
        }

        if (resultado == ESP_OK) {
            fallos_consecutivos = 0;
            hay_lectura_valida = true;
        } else {
            fallos_consecutivos++;

            if (fallos_consecutivos < DHT_MAX_FALLOS_CONSECUTIVOS) {
                ESP_LOGW(TAG, "Lectura DHT11 fallida tras %d reintentos (fallo consecutivo #%d)",
                         DHT_MAX_REINTENTOS, fallos_consecutivos);
            } else {
                ESP_LOGE(TAG, "Sensor DHT11 en GPIO%d sin respuesta tras %d ciclos consecutivos",
                         DHT_PIN, fallos_consecutivos);
            }
        }

        if (hay_lectura_valida) {
            // =========================================================
            // ACTUALIZACIÓN DE VARIABLES GLOBALES PARA EL DASHBOARD
            // =========================================================
            g_temp = (float)temperatura;
            g_hum  = (float)humedad;
            // =========================================================

            snprintf(payload_temp, sizeof(payload_temp), "%d", temperatura);
            snprintf(payload_hum, sizeof(payload_hum), "%d", humedad);
            snprintf(payload_json, sizeof(payload_json), "{\"temperatura\":%d,\"humedad\":%d}", temperatura, humedad);

            int id_temp = mqtt_manager_publish(TOPIC_PUB_TEMP, payload_temp);
            int id_hum  = mqtt_manager_publish(TOPIC_PUB_HUM, payload_hum);
            int id_json = mqtt_manager_publish(TOPIC_PUB_JSON, payload_json);

            ESP_LOGI(TAG, "DHT11 -> Temp: %d°C (ID=%d) | Hum: %d%% (ID=%d)", temperatura, id_temp, humedad, id_hum);
            ESP_LOGI(TAG, "DHT11 JSON -> %s (ID=%d)", payload_json, id_json);
        }

        vTaskDelay(pdMS_TO_TICKS(DHT_PUBLISH_PERIOD_MS));
    }
}