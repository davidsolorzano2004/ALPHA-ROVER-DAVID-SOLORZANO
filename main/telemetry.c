#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "app_config.h"
#include "mqtt_manager.h"
#include "telemetry.h"

static const char *TAG = "TELEMETRY";

// Definición de variables globales
float g_vbat1 = 0.0, g_vbat2 = 0.0, g_temp = 0.0, g_hum = 0.0, g_mic_v = 0.0;
float g_lat = 0.0, g_lon = 0.0, g_alt = 0.0;
char  g_mic_lvl[16] = "silencio";
int   g_sats = 0;
bool  g_gps_valid = false;

void get_telemetry_json(char *buf, size_t max_len) {
    snprintf(buf, max_len,
             "{\"vbat1\":%.2f,\"vbat2\":%.2f,\"temp\":%.1f,\"hum\":%.1f,\"mic_v\":%.3f,\"mic_lvl\":\"%s\","
             "\"gps\":{\"valid\":%s,\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.1f,\"sats\":%d}}",
             g_vbat1, g_vbat2, g_temp, g_hum, g_mic_v, g_mic_lvl,
             g_gps_valid ? "true" : "false", g_lat, g_lon, g_alt, g_sats);
}

void telemetry_mqtt_publisher_task(void *pvParameters) {
    char json_payload[256];
    while (1) {
        get_telemetry_json(json_payload, sizeof(json_payload));
        ESP_LOGI(TAG, "[MQTT OUT] Trama enviada: %s", json_payload);
        mqtt_manager_publish(TOPIC_PUB_TELEMETRIA, json_payload);
        vTaskDelay(pdMS_TO_TICKS(TELEMETRY_PUB_PERIOD_MS));
    }
}