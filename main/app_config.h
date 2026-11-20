#pragma once

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_adc/adc_oneshot.h"

// ============================================================================
// CONFIGURACIÓN DE RED Y BROKER MQTT (ACCESO GLOBAL)
// ============================================================================
#define WIFI_SSID "David"
#define WIFI_PASS "26082005"

// Broker MQTT público accesible por Internet
#define BROKER_URI "mqtt://broker.emqx.io:1883"

// Tópicos MQTT generales
#define TOPIC_PUB "taller2/dispositivoA/datos"
#define TOPIC_SUB "taller2/dispositivoA/control" // Tópico para recibir comandos del dashboard

// Tópicos ADC y Micrófono
#define TOPIC_PUB_ADC1 "taller2/dispositivoA/analog1"
#define TOPIC_PUB_ADC2 "taller2/dispositivoA/analog2"
#define TOPIC_PUB_MIC  "taller2/dispositivoA/microfono"

// Tópicos DHT11
#define TOPIC_PUB_TEMP "taller2/dispositivoA/temperatura"
#define TOPIC_PUB_HUM  "taller2/dispositivoA/humedad"
#define TOPIC_PUB_JSON "taller2/dispositivoA/dht11"

// Tópico de telemetría completa
#define TOPIC_PUB_TELEMETRIA   "taller2/dispositivoA/telemetria"

// Periodos de publicación (ms)
#define PUBLISH_PERIOD_MS       5000
#define ADC_PUBLISH_PERIOD_MS   1000
#define DHT_PUBLISH_PERIOD_MS   3000
#define DHT_RETRY_DELAY_MS      500
#define TELEMETRY_PUB_PERIOD_MS 3000

// ============================================================================
// CONFIGURACIÓN DE PINES Y PARÁMETROS DE SENSORES
// ============================================================================
#define DHT_PIN GPIO_NUM_16

#define ADC1_CHANNEL_ENTRADA_1 ADC_CHANNEL_6   // GPIO34
#define ADC1_CHANNEL_ENTRADA_2 ADC_CHANNEL_7   // GPIO35
#define ADC1_CHANNEL_MIC       ADC_CHANNEL_4   // GPIO32

#define ADC_NUM_MUESTRAS 64
#define MIC_NUM_MUESTRAS 200

#define MIC_UMBRAL_SILENCIO_MV   50
#define MIC_UMBRAL_BAJO_MV      150
#define MIC_UMBRAL_MEDIO_MV     500

// --- Micrófono: detección adaptativa (no depende de la ganancia del módulo) ---
// Los umbrales fijos de arriba (MIC_UMBRAL_*) ya no se usan: ahora el nivel se
// calcula comparando contra el ruido de fondo medido (el "piso" de silencio).
#define MIC_VENTANA_MS      50      // duración de cada ventana de medición
#define MIC_REPETICIONES    10      // ventanas por ciclo; se toma el pico máximo
#define MIC_RATIO_SILENCIO  1.6f    // señal < piso x 1.6  -> silencio
#define MIC_RATIO_BAJO      3.0f    // señal < piso x 3    -> bajo
#define MIC_RATIO_MEDIO     6.0f    // señal < piso x 6    -> medio  (más de eso: alto)
#define MIC_PISO_MIN_MV     4.0f    // el piso nunca baja de este valor (evita dividir por ~0)
#define MIC_PISO_SUBIDA     0.01f   // qué tan rápido sube el piso si el ambiente se vuelve más ruidoso

// BARRA DE LEDs
// SEÑALES AL ROBOCLAW
#define ID1 GPIO_NUM_4    // -> S1 (M1A/M1B)
#define ID2 GPIO_NUM_18   // -> S2 (M2A/M2B)
// #define ID3 GPIO_NUM_23
// #define ID4 GPIO_NUM_21
#define LED5_GPIO GPIO_NUM_22

#define LED_SEQUENCE_DELAY_MS 300

// ============================================================================
// CONFIGURACIÓN DE GPS U-BLOX (NEO-6M / LEA-6) (UART2) Y TELEMETRÍA
// ============================================================================
#define GPS_UART_NUM           UART_NUM_2
#define GPS_TX_PIN             GPIO_NUM_5   // ESP32 TX2 -> GPS RXD
#define GPS_RX_PIN             GPIO_NUM_17  // ESP32 RX2 <- GPS TXD
#define GPS_BAUD_RATE          9600         // Velocidad de fábrica del NEO-6M
#define GPS_BUF_SIZE           1024