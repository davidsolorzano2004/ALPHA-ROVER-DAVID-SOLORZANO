#pragma once

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

// ============================================================================
// CONFIGURACIÓN DE RED Y BROKER MQTT
// ============================================================================
#define WIFI_SSID "David"          // Reemplazar con el SSID de la red
#define WIFI_PASS "26082005"       // Reemplazar con la clave Wi-Fi
#define BROKER_URI "mqtt://broker.hivemq.com"

// Tópicos MQTT generales
#define TOPIC_PUB "taller2/dispositivoA/datos"
#define TOPIC_SUB "taller2/dispositivoB/datos"

// Tópicos ADC y Micrófono
#define TOPIC_PUB_ADC1 "taller2/dispositivoA/analog1"
#define TOPIC_PUB_ADC2 "taller2/dispositivoA/analog2"
#define TOPIC_PUB_MIC  "taller2/dispositivoA/microfono"

// Tópicos DHT11
#define TOPIC_PUB_TEMP "taller2/dispositivoA/temperatura"
#define TOPIC_PUB_HUM  "taller2/dispositivoA/humedad"
#define TOPIC_PUB_JSON "taller2/dispositivoA/dht11"

// Periodos de publicación (ms)
#define PUBLISH_PERIOD_MS     5000
#define ADC_PUBLISH_PERIOD_MS 2000
#define DHT_PUBLISH_PERIOD_MS 3000
#define DHT_RETRY_DELAY_MS 500 // margen de reposo del DHT11 antes de reintentar

// ============================================================================
// CONFIGURACIÓN DE PINES Y PARÁMETROS DE SENSORES
// ============================================================================
#define DHT_PIN GPIO_NUM_16            // GPIO16 para DHT11

// ADC1 (GPIO34, GPIO35, GPIO32)
#define ADC1_CHANNEL_ENTRADA_1 ADC_CHANNEL_6   // GPIO34
#define ADC1_CHANNEL_ENTRADA_2 ADC_CHANNEL_7   // GPIO35
#define ADC1_CHANNEL_MIC       ADC_CHANNEL_4   // GPIO32 (Módulo MH AO)

#define ADC_NUM_MUESTRAS 64
#define MIC_NUM_MUESTRAS 200

// Umbrales para micrófono (mV pico a pico) - usados en la clasificación de texto
#define MIC_UMBRAL_SILENCIO_MV   30
#define MIC_UMBRAL_BAJO_MV      100
#define MIC_UMBRAL_MEDIO_MV     300

// ============================================================================
// BARRA DE 5 LEDs (parpadeo secuencial fijo, independiente de los sensores)
// ============================================================================
// GPIOs 32, 34, 35 y 16 ya están ocupados por ADCs/DHT11. Estos 5 son pines
// de salida de uso general, no son "strapping pins" de arranque y no
// interfieren con el radio Wi-Fi/MQTT.
#define LED1_GPIO GPIO_NUM_4
#define LED2_GPIO GPIO_NUM_18
#define LED3_GPIO GPIO_NUM_23
#define LED4_GPIO GPIO_NUM_21
#define LED5_GPIO GPIO_NUM_22

// Tiempo (ms) que cada LED permanece encendido antes de pasar al siguiente
// en la secuencia. Bajar este valor = secuencia más rápida.
#define LED_SEQUENCE_DELAY_MS 300