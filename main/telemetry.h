#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdbool.h>
#include <stddef.h>

// Declaración pública de variables de telemetría
extern float g_vbat1;
extern float g_vbat2;
extern float g_temp;
extern float g_hum;
extern float g_mic_v;
extern float g_lat;
extern float g_lon;
extern float g_alt;
extern char  g_mic_lvl[16];
extern int   g_sats;
extern bool  g_gps_valid;

// Funciones públicas
void get_telemetry_json(char *buf, size_t max_len);
void telemetry_mqtt_publisher_task(void *pvParameters);

#endif // TELEMETRY_H